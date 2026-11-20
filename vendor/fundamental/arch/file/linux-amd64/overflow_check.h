#ifndef FUNDAMENTAL_FILE_OVERFLOW_CHECK_H
#define FUNDAMENTAL_FILE_OVERFLOW_CHECK_H

#include <stdint.h>
#include <stdbool.h>

static inline bool check_overflow_add(uint64_t a, uint64_t b, uint64_t *result)
{
	if (b > UINT64_MAX - a)
		return false;
	*result = a + b;
	return true;
}

static inline bool check_overflow_sub(uint64_t a, uint64_t b, uint64_t *result)
{
	if (b > a)
		return false;
	*result = a - b;
	return true;
}

static inline bool check_overflow_mul(uint64_t a, uint64_t b, uint64_t *result)
{
	if (a != 0 && b > UINT64_MAX / a)
		return false;
	*result = a * b;
	return true;
}

#endif /* FUNDAMENTAL_FILE_OVERFLOW_CHECK_H */
