#include <stdbool.h>
#include <stddef.h>

#include "fundamental/string/string.h"

/* ---- Syscall numbers ---- */
#define SYS_open 2
#define SYS_close 3
#define SYS_stat 4
#define SYS_mkdir 83
#define SYS_rmdir 84
#define SYS_getdents64 217

/* ---- open flags ---- */
#define O_RDONLY 0
#define O_DIRECTORY 0200000

/* ---- stat mode bits ---- */
#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/* ---- errno equivalents ---- */
#define EEXIST 17

/* ---- stat struct ---- */
struct stat_t {
	unsigned long st_dev;
	unsigned long st_ino;
	unsigned long st_nlink;
	unsigned int st_mode;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long st_rdev;
	long st_size;
	long st_blksize;
	long st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __pad[3];
};

/* ---- linux_dirent64 ---- */
struct linux_dirent64 {
	unsigned long long d_ino;
	long long d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[1];
};

/* ---- inline syscall helpers ---- */
static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long sys_stat(const char *path, struct stat_t *st)
{
	return syscall2(SYS_stat, (long)path, (long)st);
}

static inline bool directory_exists_raw(const char *path)
{
	struct stat_t st;
	if (sys_stat(path, &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

/* ---- Public platform API ---- */

bool fun_platform_directory_exists(const char *path)
{
	return directory_exists_raw(path);
}

bool fun_platform_directory_is_empty(const char *path)
{
	int fd = (int)syscall2(SYS_open, (long)path, O_RDONLY | O_DIRECTORY);
	if (fd < 0)
		return true;

	char buf[1024];
	bool empty = true;

	while (1) {
		int nread =
			(int)syscall3(SYS_getdents64, (long)fd, (long)buf, sizeof(buf));
		if (nread <= 0)
			break;
		int pos = 0;
		while (pos < nread) {
			struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
			const char *name = d->d_name;
			/* skip . and .. */
			if (!(name[0] == '.' &&
				  (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))) {
				empty = false;
				break;
			}
			pos += d->d_reclen;
		}
		if (!empty)
			break;
	}

	syscall1(SYS_close, (long)fd);
	return empty;
}

int fun_platform_directory_create(const char *path)
{
	long ret = syscall2(SYS_mkdir, (long)path, 0755);
	if (ret == 0)
		return 0;
	long err = -ret;
	if (err == EEXIST && directory_exists_raw(path))
		return 0;
	return -1;
}

int fun_platform_directory_remove(const char *path)
{
	if (!directory_exists_raw(path))
		return -2;
	long ret = syscall1(SYS_rmdir, (long)path);
	if (ret == 0)
		return 0;
	long err = -ret;
	if (err == 39) /* ENOTEMPTY */
		return -1;
	return -4;
}

int fun_platform_directory_list(const char *path, char *buffer,
								size_t buffer_size)
{
	if (path == NULL || buffer == NULL || buffer_size == 0)
		return -1;

	int fd = (int)syscall2(SYS_open, (long)path, O_RDONLY | O_DIRECTORY);
	if (fd < 0)
		return -3;

	size_t bytes_written = 0;
	size_t max_bytes = buffer_size - 1;
	char dents_buf[2048];

	while (1) {
		int nread = (int)syscall3(SYS_getdents64, (long)fd, (long)dents_buf,
								  sizeof(dents_buf));
		if (nread <= 0)
			break;

		int pos = 0;
		while (pos < nread) {
			struct linux_dirent64 *d =
				(struct linux_dirent64 *)(dents_buf + pos);
			const char *name = d->d_name;

			/* skip . and .. */
			if (name[0] == '.' &&
				(name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
				pos += d->d_reclen;
				continue;
			}

			StringLength entry_len = fun_string_length(name);

			// Determine type: 'D' for directory (d_type==4), 'F' for
			// others
			char type_char = (d->d_type == 4) ? 'D' : 'F';

			if (bytes_written + 2 + entry_len + 1 >= max_bytes) {
				syscall1(SYS_close, (long)fd);
				return -4;
			}

			buffer[bytes_written++] = type_char;
			buffer[bytes_written++] = '\t';
			for (StringLength i = 0; i < entry_len; i++)
				buffer[bytes_written++] = name[i];
			buffer[bytes_written++] = '\n';

			pos += d->d_reclen;
		}
	}

	syscall1(SYS_close, (long)fd);

	if (bytes_written > 0)
		bytes_written--;
	buffer[bytes_written] = '\0';

	return (int)bytes_written;
}

// ============================================================================
// Streaming Directory Handle (Walk API)
// ============================================================================

#define DIRENT_BUF_SIZE 256

typedef struct {
	int fd;
	char buf[DIRENT_BUF_SIZE];
	int buf_pos;
	int buf_len;
} LinuxDirHandle;

/* Static assertion that FUN_DIR_HANDLE_SIZE >= sizeof(LinuxDirHandle) */
typedef char
	_linux_dir_handle_size_check[640 >= sizeof(LinuxDirHandle) ? 1 : -1];

int fun_platform_dir_open(const char *path, void *handle_buf)
{
	if (path == NULL || handle_buf == NULL)
		return -1;

	LinuxDirHandle *h = (LinuxDirHandle *)handle_buf;
	h->fd = (int)syscall2(SYS_open, (long)path, O_RDONLY | O_DIRECTORY);
	if (h->fd < 0)
		return -1;
	h->buf_pos = 0;
	h->buf_len = 0;
	return 0;
}

int fun_platform_dir_read_entry(void *handle_buf, char *name_buf,
								size_t name_buf_size, char *type_out)
{
	if (handle_buf == NULL || name_buf == NULL || type_out == NULL)
		return -1;

	LinuxDirHandle *h = (LinuxDirHandle *)handle_buf;

	while (1) {
		if (h->buf_pos >= h->buf_len) {
			int nread = (int)syscall3(SYS_getdents64, (long)h->fd, (long)h->buf,
									  sizeof(h->buf));
			if (nread <= 0)
				return 0; // End of directory
			h->buf_pos = 0;
			h->buf_len = nread;
		}

		struct linux_dirent64 *d =
			(struct linux_dirent64 *)(h->buf + h->buf_pos);
		h->buf_pos += d->d_reclen;

		const char *name = d->d_name;

		// Skip . and ..
		if (name[0] == '.' &&
			(name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
			continue;
		}

		// Copy name to name_buf
		size_t i = 0;
		while (name[i] && i < name_buf_size - 1) {
			name_buf[i] = name[i];
			i++;
		}
		name_buf[i] = '\0';

		// Set type: DT_DIR == 4
		*type_out = (d->d_type == 4) ? 'D' : 'F';
		return 1;
	}
}

void fun_platform_dir_close(void *handle_buf)
{
	if (handle_buf == NULL)
		return;
	LinuxDirHandle *h = (LinuxDirHandle *)handle_buf;
	if (h->fd >= 0) {
		syscall1(SYS_close, (long)h->fd);
		h->fd = -1;
	}
}
