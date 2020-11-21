#include <stdlib.h>
#include <ctype.h>
#include "qsortHTEntries.h"
#include "getWord.h"

int compareWord(Word *word1, Word *word2)
{
   int i = 0;
   Byte byte1, byte2;
   unsigned len1 = word1 -> length, len2 = word2 -> length;
   for (i = 0; i < min(len1, len2); i++)
   {
      byte1 = word1 -> bytes[i];
      byte2 = word2 -> bytes[i];
      if (byte1 < byte2)
         return -1;
      else if (byte1 > byte2)
         return 1;
   }
   if (len1 < len2)
      return -1;
   else if (len1 > len2)
      return 1;
   return 0;
}

int compareHTEntries(const void *entry1, const void *entry2)
{
   unsigned freq1 = ((HTEntry*)entry1) -> frequency;
   unsigned freq2 = ((HTEntry*)entry2) -> frequency;
   return freq1 > freq2 ? -1 : (freq1 < freq2 ? 1 : \
      compareWord(((HTEntry*)entry1) -> data, ((HTEntry*)entry2) -> data));
}

void qsortHTEntries(HTEntry *entries, int numberOfEntries)
{
   qsort(entries, numberOfEntries, sizeof(HTEntry), compareHTEntries);
}
