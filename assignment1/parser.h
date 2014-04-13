#ifndef PARSER_H
#define PARSER_H

#define INITIAL_BUFFER 128

/*
 * Function: read_command()
 * ------------------------
 * Read a command from STDIN, limited by a newline.
 * The command has allocated memory on the stack.
 *
 * @returns command_str; An allocated string containing the commands from the prompt.
 */
char* read_command();

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
char** split_command(const char* command_str, const char* split_tok);

/*
 * Function: trim_whitespace()
 * ------------------------
 * Trims all the whitespace in the string so that
 * str[0] is the start of the first character and
 * the '\0' character is behind the last valid character.
 *
 * @returns str; An string without whitespace in the same pointer.
 */
char* trim_whitespace(char *str);

/*
 * Function: str_duplicate()
 * ------------------------
 * Returns an allocated copy of the given string.
 *
 * @param str; An string needed copying.
 * @returns return_str; An allocated string containing the given string.
 */
char* str_duplicate(const char *str);

#endif /* PARSER_H */
