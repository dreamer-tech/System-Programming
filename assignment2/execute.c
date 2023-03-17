#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "execute.h"

void flush_commands(struct cmd **cmds, int cmd_num) {
    for (int i = 0; i < cmd_num; i++) {
        free((char*)cmds[i]->name);
        for (int j = 0; j < cmds[i]->argc; j++)
            free((char*)cmds[i]->argv[j]);
        free(cmds[i]->argv);
        if (cmds[i]->in)
            free((char*)cmds[i]->in);
        if (cmds[i]->out)
            free((char*)cmds[i]->out);
        if (cmds[i]->append)
            free((char*)cmds[i]->append);
        free(cmds[i]);
    }
    free(cmds);
}

int execute_commands(struct cmd **cmds, int cmd_num) {
    if (cmd_num == 0) {
        return 0;
    }

    bool to_stop = false;
    if (cmds[cmd_num - 1]->back & OP_BACK) {
        int pid = fork();
        if (pid == 0) {
            to_stop = true;
        }
        else {
            return 0;
        }
    }

    int return_status = 0;
    for (int l = 0; l < cmd_num;) {
        int r = l;
        while (cmds[r]->next == OP_PIPE) {
            r++;
        }

        if ((l != 0 && (cmds[l - 1]->next & OP_AND) && return_status != 0) || 
            (l != 0 && (cmds[l - 1]->next & OP_OR) && return_status == 0)) {
            l = r + 1;
            continue;
        }

        int len = r - l + 1;

        if (len == 1 && strcmp(cmds[0]->name, "exit") == 0) {
            if (cmds[0]->argc == 1) {
                int exit_code = 0;
                flush_commands(cmds, cmd_num);
                exit(exit_code);
            }
            if (cmds[0]->argc == 2) {
                int exit_code = atoi(cmds[0]->argv[1]);
                flush_commands(cmds, cmd_num);
                exit(exit_code);
            }
        }


        int *pipes = (int*)malloc((len - 1) * 2 * sizeof(int));
        for (int i = 0; i < len - 1; i++) {
            pipe(&pipes[i * 2]);
        }

        int *pids = (int*)malloc(len * sizeof(int));
        memset(pids, 0, len * sizeof(int));

        for (int i = l; i <= r; i++) {
            if (strcmp(cmds[i]->name, "cd") == 0) {
                if (cmds[i]->argc == 2) {
                    chdir(cmds[i]->argv[1]);
                    return_status = 0;
                    continue;
                }
            }

            int pid = fork();

            if (pid == 0) {
                if (i != r) {
                    dup2(pipes[(i - l) * 2 + 1], 1);
                }
                if (i != l) {
                    dup2(pipes[(i - l - 1) * 2], 0);
                }
                for (int j = 0; j < len - 1; j++) {
                    close(pipes[j * 2]);
                    close(pipes[j * 2 + 1]);
                }
                if (cmds[i]->in != NULL) {
                    int fdin = open(cmds[i]->in, O_RDONLY);
                    dup2(fdin, 0);
                    close(fdin);
                }
                if (cmds[i]->out != NULL) {
                    int fdout = open(cmds[i]->out, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    dup2(fdout, 1);
                    close(fdout);
                }
                if (cmds[i]->append != NULL) {
                    int fdout = open(cmds[i]->append, O_CREAT | O_WRONLY | O_APPEND);
                    dup2(fdout, 1);
                    close(fdout);
                }
                if (strcmp(cmds[i]->name, "true") == 0) {
                    exit(0);
                }
                if (strcmp(cmds[i]->name, "false") == 0) {
                    exit(1);
                }
                if (strcmp(cmds[i]->name, "exit") == 0) {
                    if (cmds[i]->argc == 2) {
                        exit(atoi(cmds[i]->argv[0]));
                    }
                    else {
                        exit(0);
                    }
                }
                execvp(cmds[i]->name, (char* const*)cmds[i]->argv);
                exit(1);
            }
            else {
                pids[i - l] = pid;
            }
        }
        for (int i = 0; i < len - 1; i++) {
            close(pipes[i * 2]);
            close(pipes[i * 2 + 1]);
        }
        for (int i = 0; i < len; i++) {
            if (pids[i] != 0) {
                waitpid(pids[i], &return_status, 0);
            }
        }
        free(pipes);
        free(pids);

        l = r + 1;
    }

    if (to_stop) {
        exit(return_status);
    }

    return 0;
}
