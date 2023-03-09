#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"
#include "shell.h"
#include "parser.h"
#include "execute.h"

void run_shell() {
    char *str = NULL;
    size_t size;
    int status;

    // printf("$> ");
    while (status = getline(&str, &size, stdin)) {
        if (status == -1)
            break;
        
        int cmd_num;
        struct cmd **c = parse_command(str, &cmd_num);

        execute_commands(c, cmd_num);

        for (int i = 0;; i++) {
            if (c[i] == NULL)
                break;
            free((char*)c[i]->name);
            for (int j = 0; j < c[i]->argc; j++)
                free((char*)c[i]->argv[j]);
            if (c[i]->in)
                free((char*)c[i]->in);
            if (c[i]->out)
                free((char*)c[i]->out);
            if (c[i]->append)
                free((char*)c[i]->append);
            free(c[i]);
        }
        free(c);
        str = NULL;
        // printf("$> ");
    }
}
