/*
 * FILE* : parser.c
 *
 * Implements all functions needed to parse input for the shell.
 *
 * @author: Maico Timmerman
 * @uvanetid: 10542590
 * @date: 13 April 2014
 * @version: 1.0
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/*
 * Function: read_command()
 * ------------------------
 * Read a command from STDIN, limited by a newline.
 * The command has allocated memory on the stack.
 *
 * @returns command_str; An allocated string containing the commands from the prompt.
 */
char* read_command() {

    unsigned int buf_size;

    char *command_str = malloc(INITIAL_BUFFER);
    assert(command_str);
    buf_size = INITIAL_BUFFER;

    if (command_str != NULL) {

        /* Set EOF as default and character counter to 0 */
        int c = EOF;
        unsigned int i = 0;

        /* While newline is not found */
        while ((c = getchar()) != '\n') {
            /* If first character is end of file, print an extra newline. */
            if (c == EOF && i == 0) {
                fprintf(stdout, "\n");
                command_str[i] = '\0';
                return command_str;
            }
            command_str[i++] = (char)c;

            /* If the buffer is full, the double the size */
            if (i == buf_size) {
                buf_size *= 2;
                command_str = realloc(command_str, buf_size);
                assert(command_str);
            }
        }

        /* Add null terminator behind the string. */
        command_str[i] = '\0';
    }
    return command_str;
}


/*
 * Function: split_command()
 * ------------------------
 * Splits the given input string by the given token.
 * The string returned is a NULL terminated array of allocated strings.
 * The commands do not have initial or following whitespace.
 *
 * @param input_str; The string needed splitting.
 * @param split_tok; The character token used for splitting.
 * @returns commands; An allocated array of string splitted by the token.
 */
char** split_command(const char* input_str, const char* split_tok) {

    char* command_str = str_duplicate(input_str);
    char** commands = NULL;

    char* tok = strtok(command_str, split_tok);
    unsigned int n_commands = 0;

    /* While a next token is found, resize the
     * 2d array and put a copy of the toking in the array. */
    while (tok) {
        commands = realloc(commands, sizeof(char *) * ++n_commands);
        assert(commands);
        commands[n_commands - 1] = str_duplicate(trim_whitespace(tok));
        tok = strtok(NULL, split_tok);
    }

    /* Make room for the NULL terminator of the array */
    commands = realloc(commands, sizeof(char *) * ++n_commands);
    commands[n_commands - 1] = NULL;

    free(command_str);
    return commands;
}

/*
 * Function: trim_whitespace()
 * ------------------------
 * Trims all the whitespace in the string so that
 * str[0] is the start of the first character and
 * the '\0' character is behind the last valid character.
 *
 * @returns str; An string without whitespace in the same pointer.
 */
char* trim_whitespace(char *str) {
    char* start, * end;

    /* Find first non-whitespace */
    for (start = str; *start; start++) {
        if (!isspace((unsigned char)start[0]))
            break;
    }

    /* Find start of last all-whitespace */
    for (end = start + strlen(start); end > start + 1; end--) {
        if (!isspace((unsigned char)end[-1]))
            break;
    }

    /* Truncate last whitespace */
    *end = 0;

    /* Shift from "start" to the beginning of the string */
    if (start > str)
        memmove(str, start, (end - start) + 1);
    return str;
}

/*
 * Function: str_duplicate()
 * ------------------------
 * Returns an allocated copy of the given string.
 *
 * @param str; An string needed copying.
 * @returns return_str; An allocated string containing the given string.
 */
char* str_duplicate(const char *str) {
    char* return_str = malloc(strlen(str) + 1);
    if (str == NULL)
        return NULL;
    strcpy(return_str, str);
    return return_str;
}
