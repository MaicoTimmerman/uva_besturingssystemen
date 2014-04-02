#define _XOPEN_SOURCE 500

#include <assert.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"
#include "parser.h"


int main() {

    /* Set the prompt to a fancy prompt */

    while (1) {
        print_prompt();
        char* command_str = read_command();
        char ** commands = split_command(command_str, "|");
        unsigned int i = 0;

        while (commands[i]) {
            if (!strcmp(commands[i], "exit")) {
                fprintf(stdout, "received exit: <<%s>>\n", commands[i++]);
                return EXIT_SUCCESS;
            } else {
                fprintf(stdout, "<<%s>>\n", commands[i++]);
            }
        }
    }

    return EXIT_SUCCESS;
}


void print_prompt() {

    char* path = malloc((PATH_MAX + 1) * sizeof(char));
    assert(path);
    path = getcwd(path, PATH_MAX + 1);

    char* hostname = (char *)malloc((HOST_NAME_MAX + 1) * sizeof(char));
    assert(hostname);
    if (gethostname(hostname, HOST_NAME_MAX + 1))
        perror("Error: ");

    char* name = getenv("USER");

    fprintf(stdout, ANSI_COLOR_RED"%s@"ANSI_COLOR_GREEN"%s: "ANSI_COLOR_CYAN"%s" ANSI_COLOR_RESET "$ ", name, hostname,path);

}
