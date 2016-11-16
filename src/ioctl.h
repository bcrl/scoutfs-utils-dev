#ifndef _SCOUTFS_IOCTL_H_
#define _SCOUTFS_IOCTL_H_

/* XXX I have no idea how these are chosen. */
#define SCOUTFS_IOCTL_MAGIC 's'

struct scoutfs_ioctl_ino_seq {
	__u64 ino;
	__u64 seq;
} __packed;

struct scoutfs_ioctl_inodes_since {
	__u64 first_ino;
	__u64 last_ino;
	__u64 seq;
	__u64 buf_ptr;
	__u32 buf_len;
} __packed;

/*
 * Adds entries to the user's buffer for each inode whose sequence
 * number is greater than or equal to the given seq.
 */
#define SCOUTFS_IOC_INODES_SINCE _IOW(SCOUTFS_IOCTL_MAGIC, 1, \
				      struct scoutfs_ioctl_inodes_since)

/* returns bytes of path buffer set starting at _off, including null */
struct scoutfs_ioctl_ino_path {
	__u64 ino;
	__u64 ctr;		/* init to 0, set to next */
	__u64 path_ptr;
	__u16 path_bytes;	/* total buffer space, including null term */
} __packed;

/* Get a single path from the root to the given inode number */
#define SCOUTFS_IOC_INO_PATH _IOW(SCOUTFS_IOCTL_MAGIC, 2, \
				      struct scoutfs_ioctl_ino_path)

/* XXX might as well include a seq?  0 for current behaviour? */
struct scoutfs_ioctl_find_xattr {
	__u64 first_ino;
	__u64 last_ino;
	__u64 str_ptr;
	__u32 str_len;
	__u64 ino_ptr;
	__u32 ino_count;
} __packed;

#define SCOUTFS_IOC_FIND_XATTR_NAME _IOW(SCOUTFS_IOCTL_MAGIC, 3, \
				      struct scoutfs_ioctl_find_xattr)
#define SCOUTFS_IOC_FIND_XATTR_VAL _IOW(SCOUTFS_IOCTL_MAGIC, 4, \
				      struct scoutfs_ioctl_find_xattr)

#define SCOUTFS_IOC_INODE_DATA_SINCE _IOW(SCOUTFS_IOCTL_MAGIC, 5, \
					  struct scoutfs_ioctl_inodes_since)

struct scoutfs_ioctl_data_version {
	__u64 ino;
	__u64 data_version;
} __packed;

#define SCOUTFS_IOC_DATA_VERSION _IOW(SCOUTFS_IOCTL_MAGIC, 6, \
				      struct scoutfs_ioctl_data_version)

struct scoutfs_ioctl_release {
	__u64 offset;
	__u64 count;
	__u64 data_version;
} __packed;

#define SCOUTFS_IOC_RELEASE _IOW(SCOUTFS_IOCTL_MAGIC, 7, \
				  struct scoutfs_ioctl_release)

struct scoutfs_ioctl_stage {
	__u64 data_version;
	__u64 buf_ptr;
	__u64 offset;
	__s32 count;
} __packed;

#define SCOUTFS_IOC_STAGE _IOW(SCOUTFS_IOCTL_MAGIC, 8, \
			       struct scoutfs_ioctl_stage)

#endif
