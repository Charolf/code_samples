#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "helper.h"

int check_midPipe(char *temp[], int j)
{
   if (temp[j+1] != NULL)
   {
      if (strcmp(temp[j+1], "|") == SUCCESS && strcmp(temp[j], "|") == SUCCESS)
      {
         fprintf(stderr, "cshell: Invalid pipe\n");
         return ERROR;
      }
   }
   return SUCCESS;
}

int check_dirIn(char *temp[], int *j, Command *cmd_list, int cmd_count)
{
   if (strcmp(temp[*j], "<") == 0)
   {
      if (temp[*j+1] == NULL)
      {
         fprintf(stderr, "cshell: Syntax error\n");
         return ERROR;
      }
      cmd_list[cmd_count].inFile = temp[*j+1];
      (*j)++;
      return SUCCESS;
   }
   return GOON;
}

int check_dirOut(char *temp[], int *j, Command *cmd_list, int cmd_count)
{
   if (strcmp(temp[*j], ">") == 0)
   {
      if (temp[*j+1] == NULL)
      {
         fprintf(stderr, "cshell: Syntax error\n");
         return ERROR;
      }
      cmd_list[cmd_count].outFile = temp[*j+1];
      (*j)++;
      return SUCCESS;
   }
   return GOON;
}

int check_pipe(char *temp[], int j, int *arg_count, int *cmd_count)
{
   if (strcmp(temp[j], "|") == 0)
   {
      *arg_count = 0;
      (*cmd_count)++;
      if (*cmd_count >= MAX_COM)
      {
         fprintf(stderr, "cshell: Too many commands\n");
         return ERROR;
      }
      return SUCCESS;
   }
   return GOON;
}

int check_args(int *arg_count, int cmd_count, Command *cmd_list, int j, \
   char *temp[])
{
   if (*arg_count == 0)
   {
      cmd_list[cmd_count].args[(*arg_count)++] = temp[j];
      cmd_list[cmd_count].name = temp[j];
   }
   else if (*arg_count > MAX_ARG)
   {
      fprintf(stderr, "cshell: %s: Too many arguments\n", \
         cmd_list[cmd_count].name);
      return ERROR;
   }
   else
      cmd_list[cmd_count].args[(*arg_count)++] = temp[j];
   return GOON;
}

int split_cmds_helper(char *temp[], Command *cmd_list, int i, int *cmd_count)
{
   int j, status, arg_count = 0;

   for (j = 0; j < i; j++)
   {
      if (check_midPipe(temp, j) == ERROR)
         return ERROR;
      if ((status = check_dirIn(temp, &j, cmd_list, *cmd_count)) == SUCCESS)
         continue;
      else if (status == ERROR)
         return ERROR;
      if ((status = check_dirOut(temp, &j, cmd_list, *cmd_count)) == SUCCESS)
         continue;
      else if (status == ERROR)
         return ERROR;
      if ((status = check_pipe(temp, j, &arg_count, cmd_count)) == SUCCESS)
         continue;
      else if (status == ERROR)
         return ERROR;
      if (check_args(&arg_count, *cmd_count, cmd_list, j, temp) == ERROR)
         return ERROR;
   }
   return SUCCESS;
}

int split_cmds(char *command, Command *cmd_list, char *temp[])
{
   int i = 0, cmd_count = 0;
   char *token = strtok(command, " ");

   while (token != NULL)
   {
      temp[i++] = token;
      token = strtok(NULL, " ");
   }

   if (temp[0] == NULL)
      return ERROR;
   if (strcmp(temp[0], "|") == SUCCESS)
   {
      fprintf(stderr, "cshell: Invalid pipe\n");
      return ERROR;
   }

   if (split_cmds_helper(temp, cmd_list, i, &cmd_count) == ERROR)
      return ERROR;
   
   if (strcmp(temp[i-1], "|") == SUCCESS)
   {
      fprintf(stderr, "cshell: Invalid pipe\n");
      return ERROR;
   }
   if (strcmp(temp[0], "exit") == SUCCESS)
      exit(EXIT_SUCCESS);

   return cmd_count;
}

int read_line(char *command)
{
   int i = 0, ch;

   while ((ch = getchar()) != '\n' && (ch != EOF))
      command[i++] = ch;
   if (i >= MAX_LEN)
   {
      fprintf(stderr, "cshell: Command line too long\n");
      return ERROR;
   }
   if (feof(stdin) != SUCCESS)
   {
      printf("exit\n");
      exit(EXIT_SUCCESS);
   }
   return SUCCESS;
}

void new_pipe(int fd[]) /* create a new pipe */
{
   if (pipe(fd) != 0)
   {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
}

void makePipe(PipeLine *pl, int i)
{
   if ((i % 2) == 0 && i != (pl -> numProcess)) /* odd number child */
   {
      new_pipe(pl -> fd1);
      sprintf(pl -> fd1read, "%d", pl -> fd1[READ]);
      sprintf(pl -> fd1write, "%d", pl -> fd1[WRITE]);
   }
   /* even number child */
   else if ((i % 2) == 1 && i != (pl -> numProcess))
   {
      new_pipe(pl -> fd2);
      sprintf(pl -> fd2read, "%d", pl -> fd2[READ]);
      sprintf(pl -> fd2write, "%d", pl -> fd2[WRITE]);
   }
}

int openFile(const char *fileName, const char *mode)
{
   int fd, flags;

   if (0 == strcmp("r", mode))
      flags = O_RDONLY;
   else if (0 == strcmp("w", mode))
      flags = O_WRONLY | O_CREAT | O_TRUNC;
   else
   {
      fprintf(stderr, "Unknown openFile mode %s\n", mode);
      exit(EXIT_FAILURE);
   }

   if (-1 == (fd = open(fileName, flags, 0666)))
   {
      if (flags == O_RDONLY)
         fprintf(stderr, "cshell: Unable to open file for input\n");
      else
         fprintf(stderr, "cshell: Unable to open file for output\n");
      exit(EXIT_FAILURE);
   }
   return fd;
}

void check_dup2(int myFd, int oldFd)
{
   if (dup2(myFd, oldFd) == -1)
   {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
}

void childExec(int i, Command *cmd_list)
{
   int inFD, outFD;

   if (cmd_list[i].inFile != NULL)
   {
      inFD = openFile(cmd_list[i].inFile, "r");
      check_dup2(inFD, STDIN_FILENO);
   }
   if (cmd_list[i].outFile != NULL)
   {
      outFD = openFile(cmd_list[i].outFile, "w");
      check_dup2(outFD, STDOUT_FILENO);
   }

   if (execvp(cmd_list[i].name, cmd_list[i].args) < 0)
   {
      fprintf(stderr, "cshell: %s: Command not found\n", cmd_list[i].name);
      exit(EXIT_FAILURE);
   }
}

void childProcessHelper(PipeLine *pl, int i)
{
   if ((i % 2) == 0)
      check_dup2(pl -> fd2[READ], STDIN_FILENO);
   else if ((i % 2) == 1)
      check_dup2(pl -> fd1[READ], STDIN_FILENO);
}

void childProcess(PipeLine *pl, int i, Command *cmd_list)
{
   if ((pl -> numProcess) == 0); /* only 1 child */
   else if (i == 0) /* 1st child */
   {
      close(pl -> fd1[READ]);
      check_dup2(pl -> fd1[WRITE], STDOUT_FILENO);
   }
   else if (i == (pl -> numProcess)) /* last child */
      childProcessHelper(pl, i);
   else if ((i % 2) == 1) /* even number child */
   {
      close(pl -> fd2[READ]);
      check_dup2(pl -> fd1[READ], STDIN_FILENO);
      check_dup2(pl -> fd2[WRITE], STDOUT_FILENO);
   }
   else if ((i % 2) == 0) /* odd number child */
   {
      close(pl -> fd1[READ]);
      check_dup2(pl -> fd2[READ], STDIN_FILENO);
      check_dup2(pl -> fd1[WRITE], STDOUT_FILENO);
   }
   childExec(i, cmd_list);
}

void parentProcessHelper(PipeLine *pl, int i)
{
   if ((i % 2) == 0)
      close(pl -> fd2[READ]);
   else if ((i % 2) == 1)
      close(pl -> fd1[READ]);
}

void parentProcess(PipeLine *pl, int i)
{
   if ((pl -> numProcess) == 0);
   else if (i == 0)
      close(pl -> fd1[WRITE]);
   else if (i == (pl -> numProcess))
      parentProcessHelper(pl, i);
   else if ((i % 2) == 1) /* even number child */
   {
      close(pl -> fd1[READ]);
      close(pl -> fd2[WRITE]);
   }
   else if ((i % 2) == 0) /* odd number child */
   {
      close(pl -> fd2[READ]);
      close(pl -> fd1[WRITE]);
   }
}

void mainLoop(PipeLine *pl, int i, Command *cmd_list)
{
   pid_t pid;

   makePipe(pl, i);

   if ((pid = fork()) < 0) /* check fork error */
   {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
   else if (pid == 0) /* child process */
      childProcess(pl, i, cmd_list);
   else
      parentProcess(pl, i);
}
