#ifndef BUILTINS_H
#define BUILTINS_H

#define INITIAL_BUFFER 128

typedef void (*funcp_t) ();

/*
 * Function: get_builtin_command()
 * ------------------------------
 * Tests if a command is a builtin command.
 *
 * @param name; Name of the command to test for.
 * @returns funcp_t; Function pointer to the matching function.
 */
funcp_t get_builtin_command(char* name);

/*
 * Function: shell_cd()
 * ------------------------------
 * Implementation for the cd command:
 * Changes the current directory.
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_cd(char** args);

/*
 * Function: shell_exit()
 * ------------------------------
 * Implementation for the exit command:
 * Exits the shell.
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_exit(char** args);

/*
 * Function: shell_source()
 * ------------------------------
 * Implementation for the .(dot) command:
 * Execute all commands given in the filename in args[1]
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_source(char** args);


#endif /* BUILTINS_H */
