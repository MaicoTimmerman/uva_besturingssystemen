/*
 * FILE* : shell.c
 *
 * Simulates a shell in unix. Has support for piping.
 * Holds the buildins specified in builtins.c.
 *
 * @author: Maico Timmerman
 * @uvanetid: 10542590
 * @date: 13 April 2014
 * @version: 1.0
 */
#include <assert.h>
#include <limits.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "builtins.h"
#include "shell.h"
#include "parser.h"



int main() {

    fprintf(stdout, ANSI_COLOR_MAGENTA"PROMP-TARD version 1.00 pre-release alpha\n"ANSI_COLOR_RESET);

    /* Set default signal handler for the program */
    signal(SIGINT, sig_handler);

    char* command_str;

    /* Keep prompt up */
    while (1) {
        print_prompt();
        command_str = read_command();
        if (command_str[0] != '\0') {
            exec_command(command_str);
        }
        free(command_str);
    }

    return EXIT_SUCCESS;
}

/*
 * Function: exec_command()
 * ------------------------
 * Checks the command string for pipes and builtin commands.
 *
 * @param command_str; All commands given as string.
 * @returns; void.
 */
void exec_command(char* command_str) {

    funcp_t builtin_func;

    /* Split the commands on pipes */
    char** commands = split_command(command_str, "|");

    /* Split the first command to check for builtins */
    char** command_args = split_command(commands[0], " ");
    builtin_func = get_builtin_command(command_args[0]);

    /* If builtin the execute, else goto pipeline execution. */
    if (builtin_func) {
        (*builtin_func)(command_args);
    } else {
        exec_pipeline(commands, STDIN_FILENO);
    }

    /* Free all splitted commands */
    unsigned int i = 0;
    while (commands[i])
        free(commands[i++]);
    i = 0;
    while (command_args[i])
        free(command_args[i++]);
    free(commands);
}

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
void  exec_pipeline(char** commands, int in_fd) {

    char** args = split_command(commands[0], " ");
    pid_t child;

    int fd[2];
    pipe(fd);

    switch (child = fork()) {
        /* Something wrong */
        case -1:
            perror("Fork:");
            exit(EXIT_FAILURE);
        /* Child */
        case 0:
            /* Reset to the default signal handler to kill processes */
            signal(SIGINT, SIG_DFL);

            if (commands[1] != NULL) {
                /* Set the OUTPUT of child to WRITE f_desc. */
                dup2(fd[1], STDOUT_FILENO);
            }

            /* Set the INPUT of child to the given output of the previes call. */
            dup2(in_fd, STDIN_FILENO);

            /* Disobey parent. (stop reading from shell). */
            close(fd[0]);

            execvp(args[0], args);
            perror("Error:");
            _exit(EXIT_FAILURE);
        /* Parent */
        default:
            /* Parent cannot write to child. */
            close(fd[1]);
            /* If there are more commands, put them in pipeline. */
            if (commands[1] != NULL) {
                exec_pipeline(++commands, fd[0]);
            }
            /* Else wait for the last child from the pipeline to finish. */
            else {
                while (wait(NULL) != child);
            }
    }
    /* Free the splitted string */
    unsigned int i = 0;
    while (args[i])
        free(args[i++]);
    free(args);
}

/*
 * Function: print_prompt()
 * ------------------------
 * Prints the prompt with the username, computername and the current path;
 *
 * @returns; void.
 */
void print_prompt() {

    char* name = getenv("USER");
    char* hostname = getenv("HOSTNAME");

    int buff_size = 128;
    char* path = NULL;
    while (!(path = getcwd(path, buff_size))) {
        buff_size *= 2;
        path = realloc(path, buff_size * sizeof(char));
    }

    /* Print the prompt with color codes */
    fprintf(stdout,
            ANSI_COLOR_RED"%s@"ANSI_COLOR_GREEN"%s: "ANSI_COLOR_CYAN"%s" ANSI_COLOR_RESET "$ ",
            name, hostname, path);

    free(path);
}

/*
 * Function: sig_handler()
 * -----------------------
 * Does not handle the signal resets the signal handler back to itself.
 *
 * @param signo; number of the signal given.
 * @returns; void.
 */
void sig_handler(int signo) {
    signal(SIGINT, sig_handler);
}
