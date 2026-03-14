#ifndef CMD_CLEAN_H
#define CMD_CLEAN_H

/**
 * Clean command for fun CLI
 * Usage: fun clean
 * Removes build artifacts (binaries, object files)
 */

/**
 * Execute the clean command
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success, non-zero for error)
 */
int cmd_clean_execute(int argc, const char **argv);

#endif // CMD_CLEAN_H
