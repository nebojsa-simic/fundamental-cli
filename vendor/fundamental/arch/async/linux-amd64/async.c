struct timespec {
	long tv_sec;
	long tv_nsec;
};

#define CLOCK_MONOTONIC 1

static long sys_clock_gettime(int clkid, struct timespec *tp)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "0"(228L), "D"((long)clkid), "S"(tp)
						 : "rcx", "r11", "memory");
	return ret;
}

unsigned long long arch_async_now_ms(void)
{
	struct timespec ts;
	sys_clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000ULL +
		   (unsigned long long)ts.tv_nsec / 1000000ULL;
}
