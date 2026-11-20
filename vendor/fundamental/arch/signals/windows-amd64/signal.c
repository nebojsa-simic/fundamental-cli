#include "fundamental/shutdown/shutdown.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
		fun_shutdown_run(SHUTDOWN_EXTERNAL, 130);
		return TRUE;
	}
	if (ctrl_type == CTRL_CLOSE_EVENT) {
		fun_shutdown_run(SHUTDOWN_EXTERNAL, 0);
		return TRUE;
	}
	return FALSE;
}

void fun_signals_install_handlers(void)
{
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

void platform_shutdown_exit(int exit_code)
{
	ExitProcess((UINT)exit_code);
}
