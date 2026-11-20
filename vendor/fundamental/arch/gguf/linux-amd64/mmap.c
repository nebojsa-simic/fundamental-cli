#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void *_gguf_platform_open(const char *path, uint64_t *out_size,
						  void *handles[2])
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return NULL;

	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return NULL;
	}
	*out_size = (uint64_t)st.st_size;

	void *view = mmap(NULL, *out_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (view == MAP_FAILED) {
		close(fd);
		return NULL;
	}

	handles[0] = (void *)(intptr_t)fd;
	handles[1] = view;
	return view;
}

void _gguf_platform_close(void *data, void *handles[2])
{
	(void)data;
	if (handles[1])
		munmap(handles[1], 0);
	if (handles[0])
		close((int)(intptr_t)handles[0]);
}
