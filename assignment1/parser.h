//char * needs to be free'd
char* read_command();
char* trim_whitespace(char *str);
//char ** needs to be free'd
char** split_command(char* command_str, char* split_tok);
