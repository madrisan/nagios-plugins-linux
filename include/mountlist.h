#ifndef _MOUNTLIST_H
#define _MOUNTLIST_H        1

# include <stdbool.h>
# include <sys/types.h>

/* A mount table entry. */
struct mount_entry
{
  char *me_devname;             /* Device node name, including "/dev/". */
  char *me_mountdir;            /* Mount point directory name. */
  char *me_type;                /* "nfs", "4.2", etc. */
  char *me_opts;                /* Comma-separated options for fs. */
  dev_t me_dev;                 /* Device number of me_mountdir. */
  unsigned int me_dummy : 1;    /* Nonzero for dummy file systems. */
  unsigned int me_remote : 1;   /* Nonzero for remote fileystems. */
  unsigned int me_readonly : 1; /* Nonzero for readonly fileystems. */
  unsigned int me_type_malloced : 1; /* Nonzero if me_type was malloced. */
  unsigned int me_opts_malloced : 1; /* Nonzero if me_opts was malloced. */
  struct mount_entry *me_next;
};

struct mount_entry *read_file_system_list (bool need_fs_type);

#endif /* mountlist.h */
