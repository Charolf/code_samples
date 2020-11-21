#ifndef HELPER_H_
#define HELPER_H_

#define MAX_LEN 1024    /* max command line len, NOT inlude nul */
#define MAX_COM 20      /* max 20 commands */
#define MAX_ARG 10      /* max 10 args for each command */
#define ERROR -1
#define SUCCESS 0
#define GOON -2
#define READ 0 /* define READ & WRITE, much more readable! */
#define WRITE 1

typedef struct
{
   char *name, *args[MAX_ARG+1], *inFile, *outFile;
} Command;

typedef struct
{
   int fd1[2], fd2[2], numProcess;
   char fd1read[2], fd1write[2], fd2read[2], fd2write[2];
} PipeLine;

int check_args(int *arg_count, int cmd_count, Command *cmd_list, int j, \
   char *temp[]);
int check_dirIn(char *temp[], int *j, Command *cmd_list, int cmd_count);
int check_dirOut(char *temp[], int *j, Command *cmd_list, int cmd_count);
void check_dup2(int myFd, int oldFd);
int check_midPipe(char *temp[], int j);
int check_pipe(char *temp[], int j, int *arg_count, int *cmd_count);
void childExec(int i, Command *cmd_list);
void childProcessHelper(PipeLine *pl, int i);
void childProcess(PipeLine *pl, int i, Command *cmd_list);
void mainLoop(PipeLine *pl, int i, Command *cmd_list);
void makePipe(PipeLine *pl, int i);
int openFile(const char *fileName, const char *mode);
void parentProcessHelper(PipeLine *pl, int i);
void parentProcess(PipeLine *pl, int i);
int read_line(char *command);
int split_cmds_helper(char *temp[], Command *cmd_list, int i, int *cmd_count);
int split_cmds(char *command, Command *cmd_list, char *temp[]);

#endif
