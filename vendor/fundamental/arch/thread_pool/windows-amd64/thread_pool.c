#include <windows.h>

#include "fundamental/memory/memory.h"

struct arch_thread_args {
	void (*fn)(void *);
	void *arg;
};

static DWORD WINAPI arch_thread_entry(LPVOID param)
{
	struct arch_thread_args *args = (struct arch_thread_args *)param;
	void (*fn)(void *) = args->fn;
	void *arg = args->arg;

	fun_memory_free((Memory *)&param);

	fn(arg);

	return 0;
}

int arch_thread_create(void (*fn)(void *), void *arg, void **out_handle)
{
	if (out_handle == NULL) {
		return -1;
	}

	MemoryResult mem = fun_memory_allocate(sizeof(struct arch_thread_args));
	if (fun_error_is_error(mem.error)) {
		return -1;
	}

	struct arch_thread_args *args = (struct arch_thread_args *)mem.value;
	args->fn = fn;
	args->arg = arg;

	HANDLE thread = CreateThread(NULL, 0, arch_thread_entry, args, 0, NULL);
	if (thread == NULL) {
		fun_memory_free((Memory *)&mem.value);
		return -1;
	}

	*out_handle = (void *)thread;
	return 0;
}

void arch_thread_join(void *handle)
{
	if (handle == NULL) {
		return;
	}

	WaitForSingleObject((HANDLE)handle, INFINITE);
	CloseHandle((HANDLE)handle);
}
