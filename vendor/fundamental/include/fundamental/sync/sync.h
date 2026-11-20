#ifndef LIBRARY_SYNC_H
#define LIBRARY_SYNC_H

#include <stddef.h>

#include "../error/error.h"

struct Mutex_s;
typedef struct Mutex_s *Mutex;

struct CondVar_s;
typedef struct CondVar_s *CondVar;

CanReturnError(void) fun_mutex_create(Mutex *out_mutex);
voidResult fun_mutex_lock(Mutex mutex);
voidResult fun_mutex_unlock(Mutex mutex);
voidResult fun_mutex_destroy(Mutex mutex);

CanReturnError(void) fun_condvar_create(CondVar *out_cv);
voidResult fun_condvar_wait(CondVar cv, Mutex mutex);
voidResult fun_condvar_signal(CondVar cv);
voidResult fun_condvar_broadcast(CondVar cv);
voidResult fun_condvar_destroy(CondVar cv);

#endif
