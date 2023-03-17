#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "parser.h"

void remove_slash(char **str) {
    char *s = *str;
    int sz = strlen(s);
    char *t = strdup(s);

    int ptr = 0;
    for (int i = 0; i < sz; i++) {
        if (s[i] == '\\' && i + 1 < sz) {
            t[ptr] = s[i + 1];
            ptr++;
            i++;
        }
        else if (s[i] == '\\') {

        }
        else {
            t[ptr] = s[i];
            ptr++;
        }
    }
    t[ptr] = '\0';
    free(s);
    s = strdup(t);
    free(t);
    *str = s;
}

void remove_slash2(char **str) {
    char *s = *str;
    int sz = strlen(s);
    char *t = (char*)malloc((sz + 1) * sizeof(char));

    int ptr = 0;
    for (int i = 0; i < sz; i++) {
        if (s[i] == '\\' && i + 1 < sz && s[i + 1] == '\\') {
            t[ptr] = s[i];
            ptr++;
            i++;
        }
        else if (s[i] == '\\' && i + 1 < sz && s[i + 1] == '\n') {
            i++;
        }
        else {
            t[ptr] = s[i];
            ptr++;
        }
    }
    t[ptr] = '\0';
    free(s);
    s = strdup(t);
    free(t);
    *str = s;
}

#define CNT_CMD 100
#define CNT_ARG 100

struct cmd **parse_command(char *str, int *cmd_num) {
    size_t size = strlen(str);

    struct cmd **cmds = (struct cmd**)malloc(CNT_CMD * sizeof(struct cmd*));
    int ncmds = 0;

    int cnt = 0, max_cnt = CNT_ARG;
    char *name = NULL;
    char **argv = (char**)malloc(CNT_ARG * sizeof(char*));
    char *in = NULL;
    char *out = NULL;
    char *append = NULL;

    int begin = -1, end;
    int quote = 0;
    int in_begin = 0, out_begin = 0, append_begin = 0;
    int next = 0, back = 0;

    int i = 0;
    bool first_char = true;
    while (true) {
        int end_with_slash = false;

        for (; i < size; i++) {
            char cur = str[i];

            if (cur != ' ' && cur != '\n') {
                if (first_char && cur == '#') {
                    break;
                }
                first_char = false;
            }


            if ((cur == ' ' || cur == '\n' || cur == '|') && quote == 0) {
                if (begin != -1) {
                    end = i - 1;
                    if (in_begin) {
                        in = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(in, str + begin, end - begin + 1);
                        in[end - begin + 1] = '\0';
                        remove_slash(&in);
                        in_begin = 0;
                    }
                    else if (out_begin) {
                        out = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(out, str + begin, end - begin + 1);
                        out[end - begin + 1] = '\0';
                        remove_slash(&out);
                        out_begin = 0;
                    }
                    else if (append_begin) {
                        append = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(append, str + begin, end - begin + 1);
                        append[end - begin + 1] = '\0';
                        remove_slash(&append);
                        append_begin = 0;
                    }
                    else {
                        if (cnt == 0) {
                            name = (char*)malloc((end - begin + 2) * sizeof(char*));
                            strncpy(name, str + begin, end - begin + 1);
                            name[end - begin + 1] = '\0';
                            remove_slash(&name);
                        }
                        if (cnt == max_cnt - 1) {
                            max_cnt *= 2;
                            char **argv_new = (char**)malloc(max_cnt * sizeof(char*));
                            memcpy(argv_new, argv, cnt * sizeof(char*));
                            free(argv);
                            argv = argv_new;
                        }
                        argv[cnt] = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(argv[cnt], str + begin, end - begin + 1);
                        argv[cnt][end - begin + 1] = '\0';
                        remove_slash(&argv[cnt]);
                        cnt++;
                    }

                    begin = -1;
                }
            }

            if (((i + 1 < size && cur == '&' && str[i + 1] == '&') ||
                 (i + 1 < size && cur == '|' && str[i + 1] == '|') ||
                 cur == '|') && quote == 0) {
                if (i + 1 < size && cur == '&' && str[i + 1] == '&') {
                    next |= OP_AND;
                    i++;
                }
                else if (i + 1 < size && cur == '|' && str[i + 1] == '|') {
                    next |= OP_OR;
                    i++;
                }
                else {
                    next |= OP_PIPE;
                }
                cmds[ncmds] = (struct cmd*)malloc(sizeof(struct cmd));
                cmds[ncmds]->name = name;
                cmds[ncmds]->argv = (const char**)argv;
                cmds[ncmds]->argv[cnt] = NULL;
                cmds[ncmds]->argc = cnt;
                cmds[ncmds]->in = in;
                cmds[ncmds]->out = out;
                cmds[ncmds]->append = append;
                cmds[ncmds]->next = next;
                cmds[ncmds]->back = back;
                ncmds++;
                name = NULL;
                argv = (char**)malloc(CNT_ARG * sizeof(char*));
                in = NULL;
                out = NULL;
                append = NULL;
                next = 0;
                back = 0;

                cnt = 0;
                begin = -1;
            }
            else if (cur == '&' && quote == 0) {
                back |= OP_BACK;
            }
            else if (cur == '<' && quote == 0) {
                in_begin = 1;
            }
            else if (i + 1 < size && cur == '>' && str[i + 1] == '>' && quote == 0) {
                append_begin = 1;
                i++;
            }
            else if (cur == '>' && quote == 0) {
                out_begin = 1;
            }
            else if (cur == '\\' && quote == 0) {
                if (i == size - 2) {
                    end_with_slash = true;
                }
                else if (begin == -1) {
                    begin = i;
                }
                i++;
            }
            else if (cur == '\'') {
                if (quote == 0) {
                    begin = i + 1;
                    quote = 1;
                }
                else if (quote == 1) {
                    end = i - 1;

                    if (in_begin) {
                        in = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(in, str + begin, end - begin + 1);
                        in[end - begin + 1] = '\0';
                        remove_slash2(&in);
                        in_begin = 0;
                    }
                    else if (out_begin) {
                        out = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(out, str + begin, end - begin + 1);
                        out[end - begin + 1] = '\0';
                        remove_slash2(&out);
                        out_begin = 0;
                    }
                    else if (append_begin) {
                        append = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(append, str + begin, end - begin + 1);
                        append[end - begin + 1] = '\0';
                        remove_slash2(&append);
                        append_begin = 0;
                    }
                    else {
                        if (cnt == 0) {
                            name = (char*)malloc((end - begin + 2) * sizeof(char*));
                            strncpy(name, str + begin, end - begin + 1);
                            name[end - begin + 1] = '\0';
                            remove_slash2(&name);
                        }
                        if (cnt == max_cnt - 1) {
                            max_cnt *= 2;
                            char **argv_new = (char**)malloc(max_cnt * sizeof(char*));
                            memcpy(argv_new, argv, cnt * sizeof(char*));
                            free(argv);
                            argv = argv_new;
                        }
                        argv[cnt] = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(argv[cnt], str + begin, end - begin + 1);
                        argv[cnt][end - begin + 1] = '\0';
                        remove_slash2(&argv[cnt]);
                        cnt++;
                    }

                    begin = -1;
                    quote = 0;
                }
                else if (quote == 2) {
                }
            }
            else if (cur == '\"') {
                if (quote == 0) {
                    begin = i + 1;
                    quote = 2;
                }
                else if (quote == 1) {
                }
                else if (quote == 2) {
                    end = i - 1;
                    if (in_begin) {
                        in = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(in, str + begin, end - begin + 1);
                        in[end - begin + 1] = '\0';
                        remove_slash2(&in);
                        in_begin = 0;
                    }
                    else if (out_begin) {
                        out = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(out, str + begin, end - begin + 1);
                        out[end - begin + 1] = '\0';
                        remove_slash2(&out);
                        out_begin = 0;
                    }
                    else if (append_begin) {
                        append = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(append, str + begin, end - begin + 1);
                        append[end - begin + 1] = '\0';
                        remove_slash2(&append);
                        append_begin = 0;
                    }
                    else {
                        if (cnt == 0) {
                            name = (char*)malloc((end - begin + 2) * sizeof(char*));
                            strncpy(name, str + begin, end - begin + 1);
                            name[end - begin + 1] = '\0';
                            remove_slash2(&name);
                        }
                        if (cnt == max_cnt - 1) {
                            max_cnt *= 2;
                            char **argv_new = (char**)malloc(max_cnt * sizeof(char*));
                            memcpy(argv_new, argv, cnt * sizeof(char*));
                            free(argv);
                            argv = argv_new;
                        }
                        argv[cnt] = (char*)malloc((end - begin + 2) * sizeof(char*));
                        strncpy(argv[cnt], str + begin, end - begin + 1);
                        argv[cnt][end - begin + 1] = '\0';
                        remove_slash2(&argv[cnt]);
                        cnt++;
                    }

                    begin = -1;
                    quote = 0;
                }
            }
            else if (cur != ' ' && cur != '\n') {
                if (begin == -1) {
                    begin = i;
                }
            }
        }

        if (end_with_slash) {
            // printf("> ");
            char *str2 = NULL;
            size_t mem;
            int status = getline(&str2, &mem, stdin);
            if (status == -1) {
                exit(0);
            }
            int size2 = strlen(str2);
            char *t = (char*)malloc((size + size2) * sizeof(char));
            strcpy(t, str);
            strcpy(t + size - 1, str2);
            free(str);
            free(str2);
            str = t;
            size += size2 - 1;
            i--;
            continue;
        }
        if (quote != 0) {
            // printf("> ");
            char *str2 = NULL;
            size_t mem;
            int status = getline(&str2, &mem, stdin);
            if (status == -1) {
                exit(0);
            }
            int size2 = strlen(str2);
            char *t = (char*)malloc((size + size2 + 1) * sizeof(char));
            strcpy(t, str);
            strcpy(t + size, str2);
            free(str);
            free(str2);
            str = t;
            size += size2;
            continue;
        }
        break;
    }

    free(str);

    if (name) {
        cmds[ncmds] = (struct cmd*)malloc(sizeof(struct cmd));
        cmds[ncmds]->name = name;
        cmds[ncmds]->argv = (const char**)argv;
        cmds[ncmds]->argv[cnt] = NULL;
        cmds[ncmds]->argc = cnt;
        cmds[ncmds]->in = in;
        cmds[ncmds]->out = out;
        cmds[ncmds]->append = append;
        cmds[ncmds]->next = next;
        cmds[ncmds]->back = back;
        ncmds++;
    }
    else {
        free(argv);
    }
    cmds[ncmds] = NULL;
    *cmd_num = ncmds;

    return cmds;
}
