#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "cmd.h"

struct cmd **parse_command(char *str, int *cmd_num);
int execute_commands(struct cmd **c, int cmd_num);

#endif // PARSER_H_INCLUDED