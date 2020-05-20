#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "sparse.h"
#include "util.h"
#include "format.h"
#include "ioctl.h"
#include "cmd.h"

#ifndef MAX_ERRNO
#define MAX_ERRNO 4095
#endif

static int stage_cmd(int argc, char **argv)
{
	struct scoutfs_ioctl_stage args;
	unsigned int buf_len = 1024 * 1024;
	unsigned int bytes;
	char *endptr = NULL;
	char *buf = NULL;
	int afd = -1;
	int fd = -1;
	u64 offset;
	u64 count;
	u64 vers;
	int ret;

	if (argc != 6) {
		fprintf(stderr, "must specify moar args\n");
		return -EINVAL;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "failed to open '%s': %s (%d)\n",
			argv[1], strerror(errno), errno);
		return ret;
	}

	vers = strtoull(argv[2], &endptr, 0);
	if (*endptr != '\0' ||
	    ((vers == LLONG_MIN || vers == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing data version '%s'\n",
			argv[2]);
		ret = -EINVAL;
		goto out;
	}

	offset = strtoull(argv[3], &endptr, 0);
	if (*endptr != '\0' ||
	    ((offset == LLONG_MIN || offset == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing offset '%s'\n",
			argv[3]);
		ret = -EINVAL;
		goto out;
	}

	count = strtoull(argv[4], &endptr, 0);
	if (*endptr != '\0' ||
	    ((count == LLONG_MIN || count == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing count '%s'\n",
			argv[4]);
		ret = -EINVAL;
		goto out;
	}

	if (count > INT_MAX) {
		fprintf(stderr, "count %llu too large, limited to %d\n",
			count, INT_MAX);
		ret = -EINVAL;
		goto out;
	}

	afd = open(argv[5], O_RDONLY);
	if (afd < 0) {
		ret = -errno;
		fprintf(stderr, "failed to open '%s': %s (%d)\n",
			argv[5], strerror(errno), errno);
		goto out;
	}

	buf = malloc(buf_len);
	if (!buf) {
		fprintf(stderr, "couldn't allocate %u byte buffer\n", buf_len);
		ret = -ENOMEM;
		goto out;
	}

	while (count) {

		bytes = min(count, buf_len);

		ret = read(afd, buf, bytes);
		if (ret <= 0) {
			fprintf(stderr, "archive read returned %d: error %s (%d)\n",
				ret, strerror(errno), errno);
			ret = -EIO;
			goto out;
		}

		bytes = ret;

		args.data_version = vers;
		args.buf_ptr = (unsigned long)buf;
		args.offset = offset;
		args.count = bytes;

		count -= bytes;
		offset += bytes;

		ret = ioctl(fd, SCOUTFS_IOC_STAGE, &args);
		if (ret != bytes) {
			fprintf(stderr, "stage returned %d, not %u: error %s (%d)\n",
				ret, bytes, strerror(errno), errno);
			ret = -EIO;
			goto out;
		}
	}

	ret = 0;
out:
	free(buf);
	if (fd > -1)
		close(fd);
	if (afd > -1)
		close(afd);
	return ret;
};

static void __attribute__((constructor)) stage_ctor(void)
{
	cmd_register("stage", "<file> <vers> <offset> <count> <archive file>",
		     "write archive file contents to offline region", stage_cmd);
}

static int stage_err_cmd(int argc, char **argv)
{
	struct scoutfs_ioctl_stage_err args;
	u64 start_offset, end_offset;
	char *endptr = NULL;
	int fd = -1;
	s64 stage_err;
	u64 vers;
	int ret;

	if (argc != 6) {
		fprintf(stderr, "must specify moar args\n");
		return -EINVAL;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "failed to open '%s': %s (%d)\n",
			argv[1], strerror(errno), errno);
		return ret;
	}

	vers = strtoull(argv[2], &endptr, 0);
	if (*endptr != '\0' ||
	    ((vers == LLONG_MIN || vers == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing data version '%s'\n",
			argv[2]);
		ret = -EINVAL;
		goto out;
	}

	start_offset = strtoull(argv[3], &endptr, 0);
	if (*endptr != '\0' ||
	    ((start_offset == LLONG_MIN || start_offset == LLONG_MAX) &&
	     (errno == ERANGE))) {
		fprintf(stderr, "error parsing offset '%s'\n",
			argv[3]);
		ret = -EINVAL;
		goto out;
	}

	end_offset = strtoull(argv[4], &endptr, 0);
	if (*endptr != '\0' ||
	    ((end_offset == LLONG_MIN || end_offset == LLONG_MAX) &&
	     (errno == ERANGE))) {
		fprintf(stderr, "error parsing offset '%s'\n",
			argv[3]);
		ret = -EINVAL;
		goto out;
	}

	stage_err = strtoll(argv[5], &endptr, 0);
	if (*endptr != '\0' ||
	    ((stage_err == LLONG_MIN || stage_err == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing stage_err '%s'\n",
			argv[4]);
		ret = -EINVAL;
		goto out;
	}

	if ((stage_err >= 0) || (stage_err < -MAX_ERRNO)) {
		fprintf(stderr, "stage_err %lld invalid\n", stage_err);
		ret = -EINVAL;
		goto out;
	}

	memset(&args, 0, sizeof(args));
	args.data_version = vers;
	args.start_offset = start_offset;
	args.end_offset = end_offset;
	args.err = stage_err;

	ret = ioctl(fd, SCOUTFS_IOC_STAGE_ERR, &args);
	if (ret < 0) {
		fprintf(stderr, "stage returned %d: error %s (%d)\n",
			ret, strerror(errno), errno);
		ret = -EIO;
		goto out;
	}
	printf("stage_err woke %d waiters.\n", ret);

out:
	if (fd > -1)
		close(fd);
	return ret;
};

static void __attribute__((constructor)) stage_err_ctor(void)
{
	cmd_register("stage_err", "<file> <vers> <start> <end> <err>",
		     "report staging error for offline region",
		     stage_err_cmd);
}

static int release_cmd(int argc, char **argv)
{
	struct scoutfs_ioctl_release args;
	char *endptr = NULL;
	u64 block;
	u64 count;
	u64 vers;
	int ret;
	int fd;

	if (argc != 5) {
		fprintf(stderr, "must specify path, data version, offset, and count\n");
		return -EINVAL;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "failed to open '%s': %s (%d)\n",
			argv[1], strerror(errno), errno);
		return ret;
	}

	vers = strtoull(argv[2], &endptr, 0);
	if (*endptr != '\0' ||
	    ((vers == LLONG_MIN || vers == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing data version '%s'\n",
			argv[2]);
		ret = -EINVAL;
		goto out;
	}

	block = strtoull(argv[3], &endptr, 0);
	if (*endptr != '\0' ||
	    ((block == LLONG_MIN || block == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing starting 4K block offset '%s'\n",
			argv[3]);
		ret = -EINVAL;
		goto out;
	}

	count = strtoull(argv[4], &endptr, 0);
	if (*endptr != '\0' ||
	    ((count == LLONG_MIN || count == LLONG_MAX) && errno == ERANGE)) {
		fprintf(stderr, "error parsing length '%s'\n",
			argv[4]);
		ret = -EINVAL;
		goto out;
	}

	args.block = block;
	args.count = count;
	args.data_version = vers;

	ret = ioctl(fd, SCOUTFS_IOC_RELEASE, &args);
	if (ret < 0) {
		ret = -errno;
		fprintf(stderr, "release ioctl failed: %s (%d)\n",
			strerror(errno), errno);
	}
out:
	close(fd);
	return ret;
};

static void __attribute__((constructor)) release_ctor(void)
{
	cmd_register("release", "<path> <vers> <4K block offset> <block count>",
		     "mark file region offline and free extents", release_cmd);
}
