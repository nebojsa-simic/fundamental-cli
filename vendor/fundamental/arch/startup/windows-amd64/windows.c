// Fundamental Library - Windows Startup Support
// Provides command line parsing without MinGW runtime
// Enables true -nostdlib builds on Windows

// Minimal Windows API declarations
typedef void *HANDLE;
typedef unsigned long DWORD;
typedef unsigned int UINT;

#define NULL ((void *)0)

// Windows API functions we need
__declspec(dllimport) char *__stdcall GetCommandLineA(void);
__declspec(dllimport) void __stdcall ExitProcess(UINT uExitCode);

// Global argc/argv
static int g_argc = 0;
static char *g_argv[64];

// Entry point - called by linker
void main(void)
{
	// Get command line
	char *cmdline = GetCommandLineA();

	char *p = cmdline;
	g_argc = 0;

	// Parse program name as argv[0]
	g_argv[g_argc++] = p;

	// Handle quoted program name
	if (*p == '"') {
		p++;
		while (*p && *p != '"')
			p++;
		if (*p == '"') {
			*p = '\0';
			p++;
		}
	} else {
		while (*p && *p != ' ' && *p != '\t')
			p++;
		if (*p) {
			*p = '\0';
			p++;
		}
	}

	// Skip whitespace
	while (*p == ' ' || *p == '\t')
		p++;

	// Parse arguments
	while (*p && g_argc < 63) {
		g_argv[g_argc++] = p;

		if (*p == '"') {
			p++;
			while (*p && *p != '"')
				p++;
			if (*p == '"') {
				*p = '\0';
				p++;
			}
		} else {
			while (*p && *p != ' ' && *p != '\t')
				p++;
			if (*p) {
				*p = '\0';
				p++;
			}
		}

		// Skip whitespace
		while (*p == ' ' || *p == '\t')
			p++;
	}

	g_argv[g_argc] = NULL;

	// Call user's main with parsed arguments
	extern int cli_main(int argc, const char **argv);
	int result = cli_main(g_argc, (const char **)g_argv);

	// Exit process
	ExitProcess(result);
}
