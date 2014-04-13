#ifndef SHELL_H
#define SHELL_H

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


/*
 * Function: exec_command()
 * ------------------------
 * Checks the command string for pipes and builtin commands.
 *
 * @param command_str; All commands given as string.
 * @returns; void.
 */
void exec_command(char* command_str);

/*
 * Function: exec_pipeline()
 * ------------------------
 * Execute the commands given in a pipeline, where the given
 * file descriptor is the write side of the pipe precessor.
 *
 * @param commands; All commands due for executing in the pipeline
 * @param in_fd; file descriptor to read from.
 * @returns; void.
 */
void exec_pipeline(char** commands, int in_fd);

/*
 * Function: print_prompt()
 * ------------------------
 * Prints the prompt with the username, computername and the current path;
 *
 * @returns; void.
 */
void print_prompt();

/*
 * Function: sig_handler()
 * -----------------------
 * Does not handle the signal resets the signal handler back to itself.
 *
 * @param signo; number of the signal given.
 * @returns; void.
 */
void sig_handler(int signo);

#endif /* SHELL_H */
