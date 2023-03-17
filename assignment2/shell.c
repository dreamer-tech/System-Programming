#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cmd.h"
#include "shell.h"
#include "parser.h"
#include "execute.h"


void run_shell() {
    char *str = NULL;
    size_t size;
    int status;

    // printf("$> ");
    while (true) {
        status = getline(&str, &size, stdin);
        if (status == -1) {
            free(str);
            break;
        }

        int cmd_num;
        struct cmd **cmds = parse_command(str, &cmd_num);

        execute_commands(cmds, cmd_num);
        flush_commands(cmds, cmd_num);

        str = NULL;

        // printf("$> ");
    }
}
