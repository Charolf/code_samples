#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include "helper.h"

int main()
{
   int i;
   char command[MAX_LEN+1], *temp[MAX_LEN];
   Command cmd_list[MAX_COM];
   PipeLine pl;

   while (1)
   {
      memset(command, 0, MAX_LEN+1);
      memset(temp, 0, MAX_LEN*sizeof(char*));
      memset(cmd_list, 0, MAX_COM*sizeof(Command));

      printf(":-) ");
      if (read_line(command) == ERROR)
         continue;
      if ((pl.numProcess = split_cmds(command, cmd_list, temp)) == ERROR)
         continue;

      setbuf(stdout, NULL);
      for (i = 0; i < (pl.numProcess+1); i++)
         mainLoop(&pl, i, cmd_list);
      for (i = 0; i < (pl.numProcess+1); i++)
         wait(NULL);

   }
   return SUCCESS;
}
