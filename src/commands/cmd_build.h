#ifndef CMD_BUILD_H
#define CMD_BUILD_H

/**
 * Build command for fun CLI
 * Usage: fun build [--verbose] [--release] [--clean]
 */

/**
 * Execute the build command
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success, non-zero for error)
 */
int cmd_build_execute(int argc, const char **argv);

#endif // CMD_BUILD_H
