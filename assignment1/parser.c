#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define INITIAL_BUFFER 128


char* read_command() {

    unsigned int buf_size;

    char *command_str = malloc(INITIAL_BUFFER);
    buf_size = INITIAL_BUFFER;

    if (command_str != NULL) {

        /* Set EOF as default and character counter to 0 */
        int c = EOF;
        unsigned int i = 0;

        /* While newline is not found */
        while ((c = getchar()) != '\n' && c != EOF) {
            command_str[i++] = (char)c;

            /* If the buffer is full, the double the size */
            if (i == buf_size) {
                buf_size *= 2;
                command_str = realloc(command_str, buf_size);
            }
        }

        command_str[i] = '\0';
    }
    return command_str;
}


char** split_command(char* command_str, char* split_tok) {

    char* tok = command_str;
    char** commands = malloc(sizeof(char *));
    unsigned n_commands = 0;
    commands[0] = NULL;


    /* Get every token splitted by the split_tok and put it in an array with last value NULL */
    while ((tok = strtok(tok, split_tok)) != NULL) {
        commands[n_commands] = trim_whitespace(tok);
        commands = realloc(commands, (++n_commands + 1) * sizeof(char *));
        commands[n_commands] = NULL;
        tok = NULL;
    }
    return commands;
}

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
