#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#define OP_PIPE 1
#define OP_AND  2
#define OP_OR   4
#define OP_BACK 8

struct cmd {
    const char *name;
    const char **argv;
    int argc;
    const char *in;
    const char *out;
    const char *append;
    int next;
    int back;
};

#endif // EXECUTE_H_INCLUDED