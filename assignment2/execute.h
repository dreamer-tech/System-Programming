#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include "cmd.h"

void flush_commands(struct cmd **cmds, int cmd_num);
int execute_commands(struct cmd **c, int cmd_num);

#endif // EXECUTE_H_INCLUDED