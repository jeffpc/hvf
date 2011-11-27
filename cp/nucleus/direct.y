%{

#include <util.h>
#include <slab.h>
#include <directory.h>

static void merge_props(struct directory_prop *out, struct directory_prop *a,
			struct directory_prop *b)
{
	out->got_storage = a->got_storage || b->got_storage;

	if (a->got_storage && !b->got_storage)
		out->storage = a->storage;
	else if (!a->got_storage && b->got_storage)
		out->storage = b->storage;
	else if (a->got_storage && b->got_storage)
		BUG();
}

static struct directory_vdev *__alloc_spool_vdev(enum directory_vdevtype type,
						 u64 devnum, u64 typenum)
{
	struct directory_vdev *vdev;

	assert(devnum <= 0xffff);
	assert(typenum <= 0xffff);
	assert((type == VDEV_CONS) || (type == VDEV_SPOOL));

	vdev = malloc(sizeof(struct directory_vdev), ZONE_NORMAL);
	assert(vdev);

	vdev->type = type;
	vdev->vdev = devnum;

	if (type == VDEV_SPOOL) {
		vdev->u.spool.type = typenum;
		vdev->u.spool.model = 0;
	}

	return vdev;
}

static struct directory_vdev *__alloc_mdisk_vdev(u64 devnum, u64 typenum,
						 u64 start, u64 len, u64 rdev)
{
	struct directory_vdev *vdev;

	assert(devnum <= 0xffff);
	assert(typenum <= 0xffff);
	assert(start <= 0xffff);
	assert(len <= 0xffff);
	assert(rdev <= 0xffff);

	vdev = malloc(sizeof(struct directory_vdev), ZONE_NORMAL);
	assert(vdev);

	vdev->type = VDEV_MDISK;
	vdev->vdev = devnum;
	vdev->u.mdisk.cyloff = start;
	vdev->u.mdisk.cylcnt = len;
	vdev->u.mdisk.rdev = rdev;

	return vdev;
}

static int __auth_str(char *in)
{
	int a = 0;

	for(; *in; in++) {
		if (*in == 'A')
			a |= AUTH_A;
		else if (*in == 'B')
			a |= AUTH_B;
		else if (*in == 'C')
			a |= AUTH_C;
		else if (*in == 'D')
			a |= AUTH_D;
		else if (*in == 'E')
			a |= AUTH_E;
		else if (*in == 'F')
			a |= AUTH_F;
		else if (*in == 'G')
			a |= AUTH_G;
		else
			return -1;
	}

	return a;
}

static int __auth_int(u64 in)
{
	// FIXME
	return AUTH_G;
}

%}

%union {
	struct directory_vdev *vdev;
	struct list_head list;
	struct directory_prop prop;
	char *ptr;
	u64 num;
};

%token <ptr> WORD
%token <num> STORSPEC NUM
%token USER MACHINE STORAGE CONSOLE SPOOL READER PUNCH PRINT MDISK
%token NLINE COMMENT

%type <prop> props prop
%type <list> vdevs
%type <vdev> vdev

%%

users : users NLINE user
      | user
      | NLINE				/* an empty line */
      ;

user : USER WORD WORD NLINE props vdevs		{ directory_alloc_user($2, __auth_str($3), &$5, &$6); }
     | USER WORD NUM NLINE props vdevs		{ directory_alloc_user($2, __auth_int($3), &$5, &$6); }
     ;

props : props prop			{ merge_props(&$$, &$1, &$2); }
      | prop				{ memcpy(&$$, &$1, sizeof($$)); }
      ;

vdevs : vdevs vdev			{ INIT_LIST_HEAD(&$$);
					  list_splice(&$1, &$$);
					  list_add(&$2->list, &$$);
					}
      | vdev				{ INIT_LIST_HEAD(&$$);
					  list_add(&$1->list, &$$);
					}
      ;

prop : MACHINE WORD NUM NLINE		{ memset(&$$, 0, sizeof($$));
					  free($2);
					}
     | STORAGE STORSPEC NLINE		{ $$.got_storage = 1;
					  $$.storage = $2; }
     | STORAGE NUM NLINE		{ $$.got_storage = 1;
					  assert(!bcd2dec($2, &$$.storage));
					}
     ;

vdev : CONSOLE NUM NUM NLINE		{ $$ = __alloc_spool_vdev(VDEV_CONS, $2, $3); }
     | SPOOL NUM NUM READER NLINE	{ $$ = __alloc_spool_vdev(VDEV_SPOOL, $2, $3); }
     | SPOOL NUM NUM PUNCH NLINE	{ $$ = __alloc_spool_vdev(VDEV_SPOOL, $2, $3); }
     | SPOOL NUM NUM PRINT NLINE	{ $$ = __alloc_spool_vdev(VDEV_SPOOL, $2, $3); }
     | MDISK NUM NUM NUM NUM NUM NLINE	{ $$ = __alloc_mdisk_vdev($2, $3, $4, $5, $6); }
     ;
