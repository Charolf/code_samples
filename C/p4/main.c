#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "getWord.h"
#include "hashTable.h"
#include "qsortHTEntries.h"
#include "main.h"

/*
 * FNV-1a hashing Algorithm. 32-bit mode.
 *
 * http://www.isthe.com/chongo/tech/comp/fnv/index.html
 * Original Author: Glenn Fowler, Phong Vo
 * Improver:        Landon Curt Noll
 * 
 * The basis of the FNV hash algorithm was taken from an idea sent as reviewer 
 * comments to the IEEE POSIX P1003.2 committee by Glenn Fowler and Phong Vo 
 * back in 1991. In a subsequent ballot round: Landon Curt Noll improved on 
 * their algorithm. Some people tried this hash and found that it worked rather 
 * well. In an EMail message to Landon, they named it the "Fowler/Noll/Vo" or 
 * FNV hash.
 */
unsigned hash(const void *data)
{
   unsigned hash = 2166136261, len = ((Word*)data) -> length;
   Byte *str = ((Word*)data) -> bytes;
   int i;

   for (i = 0; i < len; i++)
   {
      hash ^= *str++;
      hash *= 16777619;
   }
   return hash;
}

static int compareData(const void *a, const void *b)
{
   unsigned lenA = ((Word*)a) -> length, lenB = ((Word*)b) -> length;
   return lenA > lenB ? 1 : (lenA < lenB ? -1 : memcmp(((Word*)a) -> bytes, \
      ((Word*)b) -> bytes, lenA));
}

FILE* fileOpen(const char *fname)
{
   FILE* fp;
   if ((fp = fopen(fname, "r")) == NULL)
   {
      fprintf(stderr, "wf: %s: ", fname);
      perror("");
      exit(EXIT_FAILURE);
   }
   return fp;
}

void alloc_exit(void *ptr)
{
   if (ptr == NULL)
   {
      perror("wf: ");
      exit(EXIT_FAILURE);
   }
}

void print_usage()
{
   fprintf(stderr, "Usage: wf [-nX] [file...]\n");
   exit(EXIT_FAILURE);
}

void check_arg_helper(int argc, char *argv[], int *num_line, int *flg_count)
{
   int i;
   char arg0, arg1;
   for (i = 1; i < argc; i++)
   {
      arg0 = argv[i][0];
      arg1 = argv[i][1];
      if (arg0 == '-' && arg1 != 'n')
         print_usage();
      else if (arg0 == '-' && arg1 == 'n')
      {
         if (sscanf(argv[i], "-n%d", num_line) != 1)
            print_usage();
         (*flg_count)++;
      }
   }
}

int check_arg(int argc, char* argv[], int *num_line)
{
   int flg_count = 0;

   if (argc == 1)
      return 0;
   check_arg_helper(argc, argv, num_line, &flg_count);
   if ((*num_line) < 1)
      print_usage();
   if (flg_count == (argc - 1))
      return 0;
   return 1;
}

void freeWord(const void *data)
{
   free(((Word*)data) -> bytes);
}

void create_add_free(Byte *word, unsigned length, void *ht, int hasPrintable)
{
   Word *word_struct;
   if (hasPrintable == TRUE)
   {
      word_struct = malloc(sizeof(Word));
      alloc_exit(word_struct);
      word_struct -> bytes = word;
      word_struct -> length = length;
      if (htAdd(ht, word_struct) > 1)
      {
         freeWord(word_struct);
         free(word_struct);
      }
   }
   else
      free(word);
}

void open_read_helper(FILE *file, Byte **word, unsigned *wordLength, \
   int *hasPrintable, void *ht)
{
   while(EOF != getWord(file, word, wordLength, hasPrintable))
      create_add_free(*word, *wordLength, ht, *hasPrintable);
   create_add_free(*word, *wordLength, ht, *hasPrintable);
}

void open_files(int argc, char *argv[], void *ht)
{
   Byte *word = NULL;
   unsigned wordLength = 0;
   int i, hasPrintable;
   FILE *file;
   for (i = 1; i < argc; i++)
   {
      if (argv[i][0] != '-')
      {
         file=fileOpen(argv[i]);
         open_read_helper(file, &word, &wordLength, &hasPrintable, ht);
         fclose(file);
      }
   }
}

void read_stdin(int argc, char *argv[], void *ht)
{
   Byte *word = NULL;
   unsigned wordLength = 0;
   int hasPrintable;
   open_read_helper(stdin, &word, &wordLength, &hasPrintable, ht);
}

void print_each_helper(HTEntry *entries, int i)
{
   int j;
   Byte byte;
   unsigned length = ((Word*)entries[i].data) -> length;
   printf("%10d - ", entries[i].frequency);
   for (j = 0; j < length && j < 30; j++)
   {
      byte = (((Word*)entries[i].data) -> bytes)[j];
      if (isprint(byte))
         printf("%c", byte);
      else
         printf(".");
   }
   if (length > 30)
      printf("...");
   printf("\n");
}

void print_each(HTEntry *entries, int num_line, unsigned size)
{
   int i;
   if (size < num_line)
      num_line = size;
   for (i = 0; i < num_line; i++)
      print_each_helper(entries, i);  
}

int main(int argc, char *argv[])
{
   unsigned size;
   HTEntry *entries;
   int num_line = DEFAULT, task = check_arg(argc, argv, &num_line);
   HTFunctions funcs = {hash, compareData, freeWord};
   unsigned s[] = {
      359,1579,6949,30577,134581,591901,2604347,11459087,50419883,221847497,
      976128941,4294967291
   };
   void *ht = htCreate(&funcs, s, 26, 0.7);

   if (task == 1)
      open_files(argc, argv, ht);
   else
      read_stdin(argc, argv, ht);
   entries = htToArray(ht, &size);
   qsortHTEntries(entries, size);
   printf("%d unique words found in %d total words\n", size,htTotalEntries(ht));
   print_each(entries, num_line, size);
   free(entries);
   htDestroy(ht);
   return 0;
}
