#include "fundamental/sync/sync.h"
#include "fundamental/memory/memory.h"

#define SYS_futex 202
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_PRIVATE_FLAG 128

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

struct Mutex_s {
	int32_t lock;
};

struct CondVar_s {
	int32_t seq;
};

CanReturnError(void) fun_mutex_create(Mutex *out_mutex)
{
	voidResult result;

	if (out_mutex == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	MemoryResult mem = fun_memory_allocate(sizeof(struct Mutex_s));
	if (fun_error_is_error(mem.error)) {
		result.error = mem.error;
		return result;
	}

	struct Mutex_s *m = (struct Mutex_s *)mem.value;
	m->lock = 0;

	*out_mutex = (Mutex)m;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_mutex_lock(Mutex mutex)
{
	voidResult result;
	struct Mutex_s *m = (struct Mutex_s *)mutex;

	while (1) {
		int32_t expected = 0;
		if (__atomic_compare_exchange_n(&m->lock, &expected, 1, 0,
										__ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			break;
		}
		syscall6(SYS_futex, (long)&m->lock, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1,
				 0, 0, 0);
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_mutex_unlock(Mutex mutex)
{
	voidResult result;
	struct Mutex_s *m = (struct Mutex_s *)mutex;

	__atomic_store_n(&m->lock, 0, __ATOMIC_RELEASE);
	syscall6(SYS_futex, (long)&m->lock, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0,
			 0, 0);

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_mutex_destroy(Mutex mutex)
{
	voidResult result;
	if (mutex == NULL) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}
	fun_memory_free((Memory *)&mutex);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void) fun_condvar_create(CondVar *out_cv)
{
	voidResult result;

	if (out_cv == NULL) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	MemoryResult mem = fun_memory_allocate(sizeof(struct CondVar_s));
	if (fun_error_is_error(mem.error)) {
		result.error = mem.error;
		return result;
	}

	struct CondVar_s *cv = (struct CondVar_s *)mem.value;
	cv->seq = 0;

	*out_cv = (CondVar)cv;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_wait(CondVar cv, Mutex mutex)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;
	struct Mutex_s *m = (struct Mutex_s *)mutex;

	int32_t seq = __atomic_load_n(&c->seq, __ATOMIC_ACQUIRE);

	__atomic_store_n(&m->lock, 0, __ATOMIC_RELEASE);
	syscall6(SYS_futex, (long)&m->lock, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0,
			 0, 0);

	syscall6(SYS_futex, (long)&c->seq, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, seq, 0,
			 0, 0);

	while (1) {
		int32_t expected = 0;
		if (__atomic_compare_exchange_n(&m->lock, &expected, 1, 0,
										__ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			break;
		}
		syscall6(SYS_futex, (long)&m->lock, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1,
				 0, 0, 0);
	}

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_signal(CondVar cv)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;

	__atomic_add_fetch(&c->seq, 1, __ATOMIC_RELEASE);
	syscall6(SYS_futex, (long)&c->seq, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0,
			 0);

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_broadcast(CondVar cv)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;

	__atomic_add_fetch(&c->seq, 1, __ATOMIC_RELEASE);
	syscall6(SYS_futex, (long)&c->seq, FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
			 0x7fffffff, 0, 0, 0);

	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_destroy(CondVar cv)
{
	voidResult result;
	if (cv == NULL) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}
	fun_memory_free((Memory *)&cv);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
