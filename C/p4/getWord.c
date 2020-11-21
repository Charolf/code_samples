#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "getWord.h"

#define TRUE 1
#define FALSE 0

void error_message(void *ptr)
{
   if (ptr == NULL)
   {
      perror("");
      exit(EXIT_FAILURE);
   }
}

void getWordHelper1(int ch, Byte **p, unsigned *length,int *printable,int *size)
{
   ch = tolower(ch);
   if (*length == *size)
   {
      (*size) *= 2;
      *p = realloc(*p, (*size));
      error_message(*p);
   }
   if (!isspace(ch))
      (*p)[(*length)++] = ch;
   if (isprint(ch) && !isspace(ch))
      (*printable) = TRUE;
}

void getWordHelper2(int *hasPrintable, unsigned *wordLength, int printable, \
   int length)
{
   *hasPrintable = printable;
   *wordLength = length;
   printable = length = 0;
}

int getWord(FILE *file, Byte **word, unsigned *wordLength, int *hasPrintable)
{
   int ch, printable = 0, size = 12;
   unsigned length = 0;
   Byte *p = malloc(size*sizeof(Byte));
   error_message(p);
   while ((ch = getc(file)) != EOF)
   {
      getWordHelper1(ch, &p, &length, &printable, &size);
      if (isspace(ch) && length > 0)
      {
         *word = realloc(p, length);
         error_message(*word);
         getWordHelper2(hasPrintable, wordLength, printable, length);
         return 0;
      }
   }
   *word = realloc(p, length);
   getWordHelper2(hasPrintable, wordLength, printable, length);
   return EOF;
}
