/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <ebcdic.h>
#include <binfmt_elf.h>
#include <bcache.h>
#include <edf.h>

extern u32 GUEST_IPL_CODE[];
extern u32 GUEST_IPL_REGSAVE[];

static int __copy_segment(struct virt_sys *sys, struct file *nss, u64 foff,
			  u64 fslen, u64 gaddr, u64 memlen)
{
	u32 blk, off, len;
	u64 glen;
	char *buf;
	int ret;

	con_printf(sys->con, "LOAD (%llx, %llx) -> (%llx, %llx)\n",
		   foff, fslen, gaddr, memlen);

	/* memcpy the in-file guest storage into the guest's address space */
	while(fslen) {
		blk = foff / sysfs->ADT.DBSIZ;
		off = foff % sysfs->ADT.DBSIZ;
		len = min_t(u64, sysfs->ADT.DBSIZ - off, fslen);

		con_printf(sys->con, "->   (%llx, %x) -> (%llx, %x)\n",
			   foff, len, gaddr, len);

		buf = bcache_read(nss, 0, blk);
		if (IS_ERR(buf))
			return PTR_ERR(buf);

		glen = len;
		ret = memcpy_to_guest(gaddr, buf + off, &glen);
		if (ret)
			return ret;
		BUG_ON(glen != len);

		foff   += len;
		fslen  -= len;
		gaddr  += len;
		memlen -= len;
	}

	/* memset guest storage to 0 */
	while(memlen) {
		len = min_t(u64, PAGE_SIZE, memlen);

		con_printf(sys->con, "->  0(%llx, %x)\n", gaddr, len);

		glen = len;
		ret = memcpy_to_guest(gaddr, (void*) zeropage, &glen);
		if (ret)
			return ret;
		BUG_ON(glen != len);

		memlen -= len;
		gaddr  += len;
	}

	return 0;
}

static int __nss_ipl(struct virt_sys *sys, char *cmd, int len)
{
	char fn[8];
	Elf_Ehdr *hdr;
	Elf_Phdr *phdr;
	struct file *file;
	char *buf;
	int fmt;
	int ret;
	int i;

	con_printf(sys->con, "WARNING: IPLNSS command is work-in-progress\n");

	if (len > 8)
		goto not_found;

	/* munge the system name */
	memcpy(fn, cmd, 8);
	if (len < 8)
		memset(fn + len, ' ', 8 - len);
	ascii2upper((u8*) fn, 8);

	/* look up system name on the system fs */
	file = edf_lookup(sysfs, fn, NSS_FILE_TYPE);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		if (ret == -ENOENT)
			goto not_found;
		goto err_load;
	}

	BUG_ON((file->FST.LRECL != PAGE_SIZE) || (file->FST.RECFM != FSTDFIX));

	hdr = bcache_read(file, 0, 0);
	if (IS_ERR(hdr)) {
		ret = PTR_ERR(hdr);
		goto err_load;
	}

	if ((hdr->s390.e_ident[EI_MAG0] != ELFMAG0) ||
	    (hdr->s390.e_ident[EI_MAG1] != ELFMAG1) ||
	    (hdr->s390.e_ident[EI_MAG2] != ELFMAG2) ||
	    (hdr->s390.e_ident[EI_MAG3] != ELFMAG3) ||
	    (hdr->s390.e_ident[EI_DATA] != ELFDATA2MSB) ||
	    (hdr->s390.e_ident[EI_VERSION] != EV_CURRENT) ||
	    (hdr->s390.e_type != ET_EXEC) ||
	    (hdr->s390.e_machine != EM_S390))
		goto corrupt;

	fmt = hdr->s390.e_ident[EI_CLASS];
	if ((fmt != ELFCLASS32) && (fmt != ELFCLASS64))
		goto corrupt;

	/* convert the fmt into a 'is this 64 bit system' */
	fmt = (fmt == ELFCLASS64);

	con_printf(sys->con, "NSSIPL: %s-bit system\n",
		   fmt ? "64" : "31");

	if (fmt) {
		con_printf(sys->con, "NSSIPL: '%s' is a 64-bit system - not supported\n",
			   cmd);
		goto out;
	}

	/* reset the system */
	guest_load_clear(sys);

	sys->task->cpu->state = GUEST_LOAD;

	for(i=0; i<hdr->s390.e_phnum; i++) {
		u32 blk, off;
		u64 foff, fslen, gaddr, memlen;

		foff = hdr->s390.e_phoff +
		       (hdr->s390.e_phentsize * i);
		blk  = foff / sysfs->ADT.DBSIZ;
		off  = foff % sysfs->ADT.DBSIZ;

		BUG_ON((sysfs->ADT.DBSIZ - off) < hdr->s390.e_phentsize);

		buf = bcache_read(file, 0, blk);
		if (IS_ERR(file))
			goto corrupt;

		phdr = (void*) (buf + off);

		/* skip all program headers that aren't PT_LOAD */
		if (phdr->s390.p_type != PT_LOAD)
			continue;

		if (phdr->s390.p_align != PAGE_SIZE)
			goto corrupt;

		foff   = phdr->s390.p_offset;
		fslen  = phdr->s390.p_filesz;
		gaddr  = phdr->s390.p_vaddr;
		memlen = phdr->s390.p_memsz;

		ret = __copy_segment(sys, file, foff, fslen, gaddr, memlen);
		if (ret)
			goto corrupt;
	}

	memset(&sys->task->cpu->sie_cb.gpsw, 0, sizeof(struct psw));
	sys->task->cpu->sie_cb.gpsw.fmt = 1;
	sys->task->cpu->sie_cb.gpsw.ptr31  = hdr->s390.e_entry;

out:
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;

err_load:
	sys->task->cpu->state = GUEST_STOPPED;
	con_printf(sys->con, "Error while loading NSS: %s\n",
		   errstrings[-ret]);
	return 0;

corrupt:
	sys->task->cpu->state = GUEST_STOPPED;
	con_printf(sys->con, "NSS '%s' found, but is malformed/corrupt\n", cmd);
	return 0;

not_found:
	sys->task->cpu->state = GUEST_STOPPED;
	con_printf(sys->con, "NSS '%s' does not exist\n", cmd);
	return 0;
}

static int __dev_ipl(struct virt_sys *sys, struct virt_device *vdev)
{
	u64 host_addr;
	int bytes;
	int ret;
	int i;

	/*
	 * alright, we got the device... now set up IPL helper
	 */

	bytes = sizeof(u32)*(GUEST_IPL_REGSAVE-GUEST_IPL_CODE);

	con_printf(sys->con, "WARNING: IPLDEV command is work-in-progress\n");

	/*
	 * FIXME: this should be conditional based whether or not we were
	 * told to clear
	 */
	guest_load_normal(sys);

	sys->task->cpu->state = GUEST_LOAD;

	ret = virt2phy_current(GUEST_IPL_BASE, &host_addr);
	if (ret)
		goto fail;

	/* we can't go over a page! */
	BUG_ON((bytes+(16*32)) > PAGE_SIZE);

	/* copy the helper into the guest storage */
	memcpy((void*) host_addr, GUEST_IPL_CODE, bytes);

	/* save the current guest regs */
	for (i=0; i<16; i++) {
		u32 *ptr = (u32*) ((u8*) host_addr + bytes);

		ptr[i] = (u32) sys->task->cpu->regs.gpr[i];
	}

	sys->task->cpu->regs.gpr[1]  = vdev->sch;
	sys->task->cpu->regs.gpr[2]  = vdev->pmcw.dev_num;
	sys->task->cpu->regs.gpr[12] = GUEST_IPL_BASE;

	*((u64*) &sys->task->cpu->sie_cb.gpsw) = 0x0008000080000000ULL |
						 GUEST_IPL_BASE;

	sys->task->cpu->state = GUEST_STOPPED;

	con_printf(sys->con, "GUEST IPL HELPER LOADED; ENTERED STOPPED STATE\n");

	return 0;

fail:
	sys->task->cpu->state = GUEST_STOPPED;
	return ret;
}

/*
 *!!! IPL
 *!! SYNTAX
 *! \tok{\sc IPL} <vdev>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 */
static int cmd_ipl(struct virt_sys *sys, char *cmd, int len)
{
	struct virt_device *vdev;
	u64 vdevnum = 0;
	char *c;

	/* get IPL vdev # */
	c = __extract_hex(cmd, &vdevnum);
	if (IS_ERR(c))
		goto nss;

	/* device numbers are 16-bits */
	if (vdevnum & ~0xffff)
		goto nss;

	/* find the virtual device */

	for_each_vdev(sys, vdev)
		if (vdev->pmcw.dev_num == (u16) vdevnum)
			return __dev_ipl(sys, vdev);

nss:
	/* device not found */
	return __nss_ipl(sys, cmd, len);
}

/*!!! SYSTEM CLEAR
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc CLEAR}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-clear button on a real mainframe.
 *
 *!!! SYSTEM RESET
 *!p >>--SYSTEM--RESET-------------------------------------------------------------><
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc RESET}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-normal button on a real mainframe.
 *
 *!!! SYSTEM RESTART
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc RESTART}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a restart operation.
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 *
 *!!! SYSTEM STORE
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc STORE}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 */
static int cmd_system(struct virt_sys *sys, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CLEAR")) {
		guest_system_reset_clear(sys);
		con_printf(sys->con, "STORAGE CLEARED - SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESET")) {
		guest_system_reset_normal(sys);
		con_printf(sys->con, "SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESTART")) {
		con_printf(sys->con, "SYSTEM RESTART is not yet supported\n");
	} else if (!strcasecmp(cmd, "STORE")) {
		con_printf(sys->con, "SYSTEM STORE is not yet supported\n");
	} else
		con_printf(sys->con, "SYSTEM: Unknown variable '%s'\n", cmd);

	return 0;
}
