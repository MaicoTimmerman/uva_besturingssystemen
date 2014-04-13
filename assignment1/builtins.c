/*
 * FILE* : builtins.c
 *
 * Holds all builtins for the shell.c
 *
 * @author: Maico Timmerman
 * @uvanetid: 10542590
 * @date: 13 April 2014
 * @version: 1.0
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "parser.h"
#include "shell.h"

/*
 * Function: get_builtin_command()
 * ------------------------------
 * Tests if a command is a builtin command.
 *
 * @param name; Name of the command to test for.
 * @returns funcp_t; Function pointer to the matching function.
 */
funcp_t get_builtin_command(char* name) {
    assert(name);
    if (!strcmp(name, "cd")) {
        return shell_cd;
    } else if (!strcmp(name, "exit")) {
        return shell_exit;
    } else if (!strcmp(name, ".")) {
        return shell_source;
    } else {
        return NULL;
    }
}

/*
 * Function: shell_cd()
 * ------------------------------
 * Implementation for the cd command:
 * Changes the current directory.
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_cd(char** args) {
    char* target_dir;

    /* If no dir given, goto home dir */
    if (!args[1]) {
        target_dir = getenv("HOME");
    /* Tilde goes to home dir */
    } else if (!strcmp(args[1], "~")) {
        target_dir = getenv("HOME");
    /* Else goto given dir */
    } else {
        target_dir = args[1];
    }
    if (chdir(target_dir)) {
        char error_name[FILENAME_MAX] = "promptard: cd: ";
        perror(strcat(error_name,target_dir));
    }
}

/*
 * Function: shell_exit()
 * ------------------------------
 * Implementation for the exit command:
 * Exits the shell.
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_exit(char** args) {
    exit(0);
}

/*
 * Function: shell_source()
 * ------------------------------
 * Implementation for the .(dot) command:
 * Execute all commands given in the filename in args[1]
 *
 * @param args; All arguments of the function.
 * @returns; void.
 */
void shell_source(char** args) {

    FILE* fp;

    fp = fopen(args[1], "r");
    assert(fp);

    unsigned int buf_size;

    char *command_str = malloc(INITIAL_BUFFER);
    assert(command_str);
    buf_size = INITIAL_BUFFER;


    /* Set EOF as default and character counter to 0. */
    int c = EOF;
    unsigned int i = 0;

    /* Until the end of the file, execute all commands seperated by newlines. */
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') {
            command_str[i] = '\0';
            if (command_str[0] != '\0') {
                command_str = trim_whitespace(command_str);
                /* Ignore exit commands */
                if (!strncmp("exit", command_str, 4))
                    continue;
                exec_command(command_str);
            }
            i = 0;
        }
        else {
            command_str[i++] = (char)c;
            /* If the buffer is full, the double the size */
            if (i == buf_size) {
                buf_size *= 2;
                command_str = realloc(command_str, buf_size);
                assert(command_str);
            }
        }

    }

    free(command_str);
}
