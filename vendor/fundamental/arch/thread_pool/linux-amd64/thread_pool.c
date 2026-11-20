#include "fundamental/memory/memory.h"

#define SYS_clone 56
#define SYS_futex 202
#define SYS_exit 60

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_PRIVATE_FLAG 128

#define CLONE_VM 0x00000100
#define CLONE_FS 0x00000200
#define CLONE_FILES 0x00000400
#define CLONE_SIGHAND 0x00000800
#define CLONE_THREAD 0x00010000
#define CLONE_SYSVSEM 0x00040000
#define CLONE_CHILD_CLEARTID 0x00200000
#define CLONE_CHILD_SETTID 0x01000000

#define THREAD_STACK_SIZE (128 * 1024)

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5,
							long a6)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
						   "r"(r9)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline void futex_wait(int *addr, int val)
{
	syscall6(SYS_futex, (long)addr, FUTEX_WAIT, (long)val, 0, 0, 0);
}

static inline void futex_wake(int *addr, int count)
{
	syscall6(SYS_futex, (long)addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
			 (long)count, 0, 0, 0);
}

struct linux_thread_handle {
	int tid;
	int clear_tid;
	void *stack;
};

int arch_thread_create(void (*fn)(void *), void *arg, void **out_handle)
{
	if (out_handle == NULL) {
		return -1;
	}

	MemoryResult stack_mem = fun_memory_allocate(THREAD_STACK_SIZE);
	if (fun_error_is_error(stack_mem.error)) {
		return -1;
	}

	MemoryResult handle_mem =
		fun_memory_allocate(sizeof(struct linux_thread_handle));
	if (fun_error_is_error(handle_mem.error)) {
		fun_memory_free((Memory *)&stack_mem.value);
		return -1;
	}

	struct linux_thread_handle *h =
		(struct linux_thread_handle *)handle_mem.value;
	h->stack = stack_mem.value;
	h->clear_tid = 0;

	void *stack_top = (void *)((char *)stack_mem.value + THREAD_STACK_SIZE);

	/* Store fn/arg on child's own stack so child can access them
	   without depending on parent's stack frame (which may use
	   RSP-relative addressing that breaks after clone switches RSP). */
	void **child_data = (void **)stack_top - 2;
	child_data[0] = (void *)fn;
	child_data[1] = arg;

	unsigned long flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
						  CLONE_THREAD | CLONE_SYSVSEM | CLONE_CHILD_CLEARTID |
						  CLONE_CHILD_SETTID;

	long tid;
	/* Inline clone + child execution.
	   The child MUST run entirely inside this asm block: any C code
	   after clone() would execute with the wrong RSP because the
	   compiler's function prologue has already shifted RSP on the
	   parent's stack.  Only raw asm sees the fresh child_stack. */
	__asm__ __volatile__(
		"mov %[nr], %%rax\n\t"
		"mov %[flags], %%rdi\n\t"
		"mov %[stack], %%rsi\n\t"
		"mov %[ptid], %%rdx\n\t"
		"mov %[ctid], %%r10\n\t"
		"mov %[tls], %%r8\n\t"
		"syscall\n\t"
		"test %%rax, %%rax\n\t"
		"jnz 1f\n\t"
		/* ---- child ---- */
		"movq 0(%%rsp), %%rax\n\t" /* fn */
		"movq 8(%%rsp), %%rdi\n\t" /* arg */
		"call *%%rax\n\t"
		"mov %[exit_nr], %%eax\n\t"
		"xor %%edi, %%edi\n\t"
		"syscall\n\t"
		/* ---- parent ---- */
		"1:\n\t"
		"mov %%rax, %[ret]\n\t"
		: [ret] "=r"(tid)
		: [nr] "i"(SYS_clone), [flags] "r"(flags), [stack] "r"((long)child_data),
		  [ptid] "r"(0UL), [ctid] "r"((long)&h->clear_tid), [tls] "r"(0UL),
		  [exit_nr] "i"(SYS_exit)
		: "rax", "rdi", "rsi", "rdx", "r10", "r8", "rcx", "r11", "memory");

	if (tid < 0) {
		fun_memory_free((Memory *)&stack_mem.value);
		fun_memory_free((Memory *)&handle_mem.value);
		return -1;
	}

	h->tid = (int)tid;
	h->clear_tid = (int)tid;
	*out_handle = h;
	return 0;
}

void arch_thread_join(void *handle)
{
	if (handle == NULL) {
		return;
	}

	struct linux_thread_handle *h = (struct linux_thread_handle *)handle;

	int tid = h->tid;
	if (tid > 0) {
		while (__atomic_load_n(&h->clear_tid, __ATOMIC_ACQUIRE) != 0) {
			futex_wait(&h->clear_tid, tid);
		}
	}

	fun_memory_free((Memory *)&h->stack);
	fun_memory_free((Memory *)&handle);
}
