/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <mm.h>
#include <bcache.h>
#include <edf.h>
#include <vcpu.h>
#include <guest.h>
#include <ebcdic.h>
#include <binfmt_elf.h>

static int __copy_segment(struct virt_sys *sys, struct file *nss, u64 foff,
			  u64 fslen, u64 gaddr, u64 memlen)
{
	u32 blk, off, len;
	u64 glen;
	char *buf;
	int ret;

	/* memcpy the in-file guest storage into the guest's address space */
	while(fslen) {
		blk = foff / sysfs->ADT.DBSIZ;
		off = foff % sysfs->ADT.DBSIZ;
		len = min_t(u64, sysfs->ADT.DBSIZ - off, fslen);

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


int guest_ipl_nss(struct virt_sys *sys, char *nssname)
{
	char fn[8];
	Elf_Ehdr *hdr;
	Elf_Phdr *phdr;
	struct file *file;
	char *buf;
	int fmt;
	int len;
	int ret;
	int i;

	len = strnlen(nssname, sizeof(fn)+1);

	if (len > 8)
		goto not_found;

	/* munge the system name */
	memcpy(fn, nssname, 8);
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

	assert((file->FST.LRECL == PAGE_SIZE) && (file->FST.RECFM == FSTDFIX));

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

	/* FIXME: 31 bits systems aren't supported */
	if (fmt)
		goto out;

	/* reset the system */
	guest_load_clear(sys);

	sys->cpu->state = GUEST_LOAD;

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

	memset(&sys->cpu->sie_cb.gpsw, 0, sizeof(struct psw));
	sys->cpu->sie_cb.gpsw.fmt = 1;
	sys->cpu->sie_cb.gpsw.ptr31  = hdr->s390.e_entry;

out:
	sys->cpu->state = GUEST_STOPPED;
	return 0;

err_load:
	sys->cpu->state = GUEST_CHECKSTOP;
	return -EIO;

corrupt:
	sys->cpu->state = GUEST_CHECKSTOP;
	return -ECORRUPT;

not_found:
	sys->cpu->state = GUEST_CHECKSTOP;
	return -ENOENT;
}
