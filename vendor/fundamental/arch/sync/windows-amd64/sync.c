#include "fundamental/sync/sync.h"
#include "fundamental/memory/memory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Mutex_s {
	CRITICAL_SECTION cs;
};

struct CondVar_s {
	CONDITION_VARIABLE cv;
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
	InitializeCriticalSection(&m->cs);

	*out_mutex = (Mutex)m;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_mutex_lock(Mutex mutex)
{
	voidResult result;
	struct Mutex_s *m = (struct Mutex_s *)mutex;
	EnterCriticalSection(&m->cs);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_mutex_unlock(Mutex mutex)
{
	voidResult result;
	struct Mutex_s *m = (struct Mutex_s *)mutex;
	LeaveCriticalSection(&m->cs);
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
	struct Mutex_s *m = (struct Mutex_s *)mutex;
	DeleteCriticalSection(&m->cs);
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
	InitializeConditionVariable(&cv->cv);

	*out_cv = (CondVar)cv;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_wait(CondVar cv, Mutex mutex)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;
	struct Mutex_s *m = (struct Mutex_s *)mutex;
	SleepConditionVariableCS(&c->cv, &m->cs, INFINITE);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_signal(CondVar cv)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;
	WakeConditionVariable(&c->cv);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

voidResult fun_condvar_broadcast(CondVar cv)
{
	voidResult result;
	struct CondVar_s *c = (struct CondVar_s *)cv;
	WakeAllConditionVariable(&c->cv);
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
	struct CondVar_s *c = (struct CondVar_s *)cv;
	(void)c;
	fun_memory_free((Memory *)&cv);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
