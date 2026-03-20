#include <stdbool.h>
#include <stddef.h>

#define S_IFMT  0170000
#define S_IFDIR 0040000
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

struct stat_t {
	unsigned long st_dev;
	unsigned long st_ino;
	unsigned long st_nlink;
	unsigned int  st_mode;
	unsigned int  st_uid;
	unsigned int  st_gid;
	unsigned long st_rdev;
	long          st_size;
	long          st_blksize;
	long          st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __pad[3];
};

static inline long sys_stat(const char *path, struct stat_t *st)
{
	long ret;
	__asm__ __volatile__(
		"syscall"
		: "=a"(ret)
		: "0"(4L), "D"(path), "S"(st)
		: "rcx", "r11", "memory");
	return ret;
}

bool fun_platform_file_exists(const char *path)
{
	if (path == NULL)
		return false;
	struct stat_t st;
	if (sys_stat(path, &st) != 0)
		return false;
	return !S_ISDIR(st.st_mode);
}

bool fun_platform_path_exists(const char *path)
{
	if (path == NULL)
		return false;
	struct stat_t st;
	return sys_stat(path, &st) == 0;
}
