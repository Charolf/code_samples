#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "hashTable.h"
#include "getWord.h"

#define FALSE 0
#define TRUE  1
/*
 * This is the fundamental type of a hash table using separate
 * chaining as its collision strategy. It will only be used by the
 * hash table so it can be defined in your hash table source file so
 * that it is private/file local.
 */
typedef struct node
{
   /* Other unspecified fields you deem necessary here... */
   void *data;
   unsigned frequency;
   unsigned hash;
   /* The quintisential "next" pointer */
   struct node *next;
} HashNode;

/*
 * This is the structure representing a hash table. It will be defined
 * in your hash table source file so that it is private/file local. And,
 * you will be allocating dynamic memory for one so that you can return
 * a void* to one from the htCreate "constructor-like" function.
 *
 * Notice that theArray is a double-pointer. That is because it is an
 * array (a pointer) of HashNode pointers!
 */
typedef struct
{
   /* Other unspecified fields you deem necessary here... */
   HTFunctions *theFunctions;
   unsigned *theSizes;
   unsigned rehash;
   unsigned unique;
   unsigned total;
   float loadFactor;
   int sizes;
   /* The quintessential "hash table", a.k.a., an array of node pointers */
   HashNode **theArray;
} HashTable;

/* 
 * malloc failure caller function.
 */
void alloc_message(void *pointer)
{
   if (pointer == NULL)
   {
      fprintf(stderr, "calloc failure in %s at %d\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
   }
}

/* Description: Creates a new hash table as specified.
 *
 * Notes:
 *   1. The function asserts (man 3 assert) if sizes is not 1 or more.
 *   2. The function asserts (man 3 assert) if any of the sizes are not
 *      greater than the immediately preceding size.
 *   3. The function asserts (man 3 assert) If the load factor is not greater
 *      than 0.0 and less than or equal to 1.0.
 *   4. You must make a deep copy of the _functions_ pointers and _sizes_
 *      array since you don't know where the caller allocated them - they
 *      could be on the stack and you don't want to point there - that's a
 *      potential and likely _unlucky bug_ waiting to happen!
 *
 * Parameters:
 *    functions: A structure of function pointers to the data-specific
 *       functions required by the hash table.
 *    sizes: An array of hash table sizes. Must always be at least one size
 *       greater than zero specified, more if rehashing is desired. Prime
 *       numbers are best but not required/checked for. That being said, you
 *       should use prime numbers!
 *    numSizes: The number of values in the sizes array.
 *    rehashLoadFactor: The load factor to rehash at. Should be a value greater
 *       than 0.0 and less than or equal to 1.0. A value of 1.0 means
 *       "do not rehash".
 *
 * Return: A pointer to an anonymous (file-local) structure representing a
 *         hash table.
 */

void* htCreate(HTFunctions *functions, unsigned sizes[], int numSizes, \
   float rehashLoadFactor)
{
   int i;
   HashTable *hashTable;

   assert(numSizes >= 1);
   assert(sizes[0] >= 1);
   if (numSizes > 1)
   {
      for (i = 1; i < numSizes; i++)
         assert(sizes[i] > sizes[i-1]);
   }
   assert(rehashLoadFactor > 0 && rehashLoadFactor <= 1);

   hashTable = (HashTable*)malloc(sizeof(HashTable));
   alloc_message(hashTable);

   hashTable -> theFunctions = (HTFunctions*)malloc(sizeof(HTFunctions));
   alloc_message(hashTable -> theFunctions);
   memcpy(hashTable -> theFunctions, functions, sizeof(HTFunctions));

   hashTable -> theSizes = (unsigned*)malloc(numSizes*sizeof(unsigned));
   alloc_message(hashTable -> theSizes);
   memcpy(hashTable -> theSizes, sizes, numSizes*sizeof(unsigned));
   
   hashTable -> theArray = calloc(sizes[0], sizeof(HashNode*));
   alloc_message(hashTable -> theArray);

   hashTable -> rehash = 0;
   hashTable -> unique = 0;
   hashTable -> total = 0;
   hashTable -> loadFactor = rehashLoadFactor;
   hashTable -> sizes = numSizes;

   return hashTable;
}

/* Description: Frees all of the dynamically allocated memory allocated by the
 *    hash table itself as well as the all of the data object added to the
 *    hash table by the user. Note that when a destroy function was provided to
 *    htCreate, it is called on each data object added by the user prior to
 *    freeing that object.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *
 * Return: None
 */
void htDestroy_helper(void *hashTable, HashNode *the_node)
{
   HashNode *temp_node;
   FNDestroy destroyFunc = ((HashTable*)hashTable) -> theFunctions -> destroy;

   while (the_node != NULL)
   {
      if (destroyFunc != NULL)
         destroyFunc(the_node -> data);
      temp_node = the_node;
      the_node = the_node -> next;
      free(temp_node -> data);
      free(temp_node);
   }
}

void htDestroy(void *hashTable)
{
   int i;
   HashNode *the_node;
   unsigned *sizesArray = ((HashTable*)hashTable) -> theSizes;
   unsigned rehashCount = ((HashTable*)hashTable) -> rehash;
   HashNode **nodeArray = ((HashTable*)hashTable) -> theArray;
   HTFunctions *hereFunctions = ((HashTable*)hashTable) -> theFunctions;

   for (i = 0; i < sizesArray[rehashCount]; i++)
   {
      the_node = nodeArray[i];
      htDestroy_helper(hashTable, the_node);
   }
   free(nodeArray);
   free(sizesArray);
   free(hereFunctions);
   free((HashTable*)hashTable);
}

/* Description: Adds a shallow copy of the data to the hash table. The data
 *    being added MUST BE dynamically allocated because it will be freed
 *    when the hash table is destroyed!
 * 
 * Notes:
 *    1. The function is expected to have O(1) performance.
 *    2. The function asserts (man 3 assert) if data is NULL. 
 *    3. The function rehashes to the next size when:
 *          1. The rehash load factor is NOT 1.0, a value of 1.0 indicates
 *             rehashing is not desired.
 *          2. And there is a next size. Otherwise continue with the current
 *             size.
 *          3. And the ratio of unique entries TO the current hash table size
 *             (BEFORE adding the new data) exceeds the rehash load factor.
 *    4. When the data being added is a duplicate, the original entry is kept
 *       in the hash table AND the caller is responsible for freeing the
 *       duplicate.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *    data: The data to add.
 *
 * Return: The frequency of the data in the hash table. A value of 1 means it
 *    is a new and unique entry, values greater than 1 mean it is a duplicate
 *    with the indicated frequency.
 */
void rehashHelper(void *hashTable, HashNode *the_node, HashNode **newArray)
{
   HashNode *temp_node;
   unsigned new_index;

   while (the_node != NULL)
   {
      temp_node = the_node -> next;
      new_index = (the_node -> hash) % htCapacity(hashTable);
      the_node -> next = newArray[new_index];
      newArray[new_index] = the_node;
      the_node = temp_node;
   }
}

void rehash(void *hashTable, unsigned herehHash, int hereSizes)
{
   int i;
   HashNode *the_node;
   HashNode **newArray, **temp_array;
   unsigned *hereListSizes = ((HashTable*)hashTable) -> theSizes;

   ((HashTable*)hashTable) -> rehash = herehHash + 1;
   newArray = calloc(hereListSizes[((HashTable*)hashTable) -> rehash], \
      sizeof(HashNode*));
   alloc_message(newArray);

   for (i = 0; i < hereListSizes[herehHash]; i++)
   {
      the_node = (((HashTable*)hashTable) -> theArray)[i];
      rehashHelper(hashTable, the_node, newArray);
   }
   temp_array = ((HashTable*)hashTable) -> theArray;
   ((HashTable*)hashTable) -> theArray = newArray;
   free(temp_array);
}

unsigned addData(void *hashTable, void *data, unsigned raw_hash, int *decider)
{
   HashNode *current, *new, **nextp;
   HashNode **hereAray = ((HashTable*)hashTable) -> theArray;
   FNCompare compareFunc = ((HashTable*)hashTable) -> theFunctions -> compare;
   nextp = &hereAray[raw_hash % htCapacity(hashTable)];

   while ((current = *nextp) != NULL)
   {
      if (compareFunc(current -> data, data) == 0)
      {
         (current -> frequency)++;
         (((HashTable*)hashTable) -> total)++;
         *decider = TRUE;
         return (current -> frequency);
      }
      nextp = &current -> next;
   }
   new = (HashNode*)malloc(sizeof(HashNode));
   alloc_message(new);
   new -> data = data;
   new -> frequency = 1;
   new -> hash = raw_hash;
   new -> next = current;
   *nextp = new;
   *decider = FALSE;
   return (new -> frequency);
}

unsigned htAdd(void *hashTable, void *data)
{
   int decider;
   unsigned freq;
   float hereFactor = ((HashTable*)hashTable) -> loadFactor;
   int hereSizes = ((HashTable*)hashTable) -> sizes;
   unsigned herehHash = ((HashTable*)hashTable) -> rehash;

   assert(data != NULL);

   if ((hereFactor < 1) && (herehHash < (hereSizes - 1)) && \
      (((float)htUniqueEntries(hashTable) / htCapacity(hashTable)) >hereFactor))
      rehash(hashTable, herehHash, hereSizes);

   freq = addData(hashTable, data, (((HashTable*)hashTable) -> theFunctions -> \
      hash)(data), &decider);
   if (decider == TRUE)
      return freq;
   (((HashTable*)hashTable) -> unique)++;
   (((HashTable*)hashTable) -> total)++;

   return freq;
}

/* Description: Determines if the data is in the hash table or not.
 * 
 * Notes:
 *    1. The function is expected to have O(1) performance.
 *    2. The function asserts (man 3 assert) if data is NULL. 
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *    data: The data to look for.
 *
 * Return: Returns an HTEntry with a shallow copy of the data and its
 *    frequency if found, otherwise NULL data and frequency 0.
 */
HTEntry htLookUp(void *hashTable, void *data)
{
   HashNode *the_node;
   HTEntry the_entry;
   unsigned true_hash;
   HTFunctions *theFunctions = ((HashTable*)hashTable) -> theFunctions;

   assert(data != NULL);
   the_entry.data = NULL;
   the_entry.frequency = 0;
   true_hash = (theFunctions -> hash)(data) % htCapacity(hashTable);
   the_node = (((HashTable*)hashTable) -> theArray)[true_hash];
   while (the_node != NULL)
   {
      if ((theFunctions -> compare)(the_node -> data, data) == 0)
      {
         the_entry.data = the_node -> data;
         the_entry.frequency = the_node -> frequency;
         return the_entry;
      }
      the_node = the_node -> next;
   }
   return the_entry;
}

/* Description: Returns a dynamically allocated array with shallow copies of
 * all of the hash table entries.
 *
 * Notes:
 *    1. The caller is responsible for freeing the memory for the returned
 *       array BUT MUST NOT free the data in each HTEntry in the array - that
 *       memory is still in use by the hash table! To free the data call
 *       htDestroy when you are completely done with the hash table AND all
 *       arrays returned by this function.
 * 
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *    size: Output parameter updated with the array's size.
 *
 * Return: A dynamically allocated array with all of the hash table entries or
 *    NULL if the hash table is empty (note that free can be called on NULL).
 */
void htToArrayHelper(void *hashTable, int i, int *j, HTEntry *entryArray)
{
   HashNode *current = (((HashTable*)hashTable) -> theArray)[i];

   while (current != NULL)
   {
      entryArray[*j].data = current -> data;
      entryArray[*j].frequency = current -> frequency;
      (*j)++;
      current = current -> next;
   }
}

HTEntry* htToArray(void *hashTable, unsigned *size)
{
   int i, j = 0;
   unsigned unique_count = htUniqueEntries(hashTable);
   unsigned capacity = htCapacity(hashTable);
   HTEntry *entryArray;

   *size = unique_count;

   if (unique_count == 0)
   {
      entryArray = NULL;
      return entryArray;
   }
   entryArray = calloc(unique_count, sizeof(HTEntry));
   alloc_message(entryArray);
   for (i = 0; i < capacity; i++)
      htToArrayHelper(hashTable, i, &j, entryArray);
   return entryArray;
}

/* Description: Reports the current capacity of the hash table.
 * 
 * Notes:
 *    1. The function is expected to have O(1) performance.
 *    2. This function should always return one of the sizes passed to htCreate.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *
 * Return: The current capacity of the hash table.
 */
unsigned htCapacity(void *hashTable)
{
   return (((HashTable*)hashTable)->theSizes)[((HashTable*)hashTable)-> rehash];
}

/* Description: Returns the number of unique entries in the hash table.
 * 
 * Notes: 
 *    1. The function is expected to have O(1) performance which implies it must
 *       be stored and maintained by the hash table as data is added - NOT
 *       calculated when this function is called.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *
 * Return: The number of unique entries in the hash table.
 */
unsigned htUniqueEntries(void *hashTable)
{
   return ((HashTable*)hashTable) -> unique;
}

/* Description: Returns the total number of data objects added to the hash
 *    table which is equivalent to the sum of the frequencies of all entries
 *    in the hash table (see Notes, below, for important information about
 *    the expected performance of this function).
 * 
 * Notes: 
 *    1. The function is expected to have O(1) performance which implies it must
 *       be stored and maintained by the hash table as data is added - NOT
 *       calculated when this function is called.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *
 * Return: The sum of the frequencies of all entries in the hash table.
 */
unsigned htTotalEntries(void *hashTable)
{
   return ((HashTable*)hashTable) -> total;
}

/* Description: Returns various metrics on the hash table to aid in performance
 *    tuning of CPU and memory usage for a particular problem domain.
 * 
 * Notes:
 *    1. Because this function will only be used for performance tuning and not
 *       during actual use of the hash table it DOES NOT need to have O(1)
 *       performance. FYI: O(N) performance is expected.
 *    2. The number of chains is a count of the non-empty chains in the
 *       hash table.
 *    3. The maximum chain length is the length of the longest chain in the
 *       hash table.
 *    4. The average chain length is the sum of the length of all chains
 *       in the hash table divided by the number of non-zero length chains.
 *
 * Parameters:
 *    hashTable: A pointer returned by htCreate.
 *
 * Return: An HTMetric struct with the current metrics for the specified hash
 *    table.
 */
void htMetricsHelper2(HashNode *current, unsigned *this_len)
{
   while (current != NULL)
   {
      (*this_len)++;
      current = current -> next;
   }
}

void htMetricsHelper(void *hashTable, int i, unsigned *num_chains, \
   unsigned *sum_len, unsigned *max_len)
{
   HashNode *current;
   unsigned this_len = 0;

   if ((current = (((HashTable*)hashTable) -> theArray)[i]) != NULL)
      (*num_chains)++;
   htMetricsHelper2(current, &this_len);
   (*sum_len) += this_len;
   if (this_len > *max_len)
      *max_len = this_len;
}

HTMetrics htMetrics(void *hashTable)
{
   int i;
   unsigned sum_len = 0;
   unsigned capacity = htCapacity(hashTable);
   HTMetrics metrics;

   metrics.numberOfChains = 0;
   metrics.maxChainLength = 0;

   for (i = 0; i < capacity; i++)
      htMetricsHelper(hashTable, i, &(metrics.numberOfChains), &sum_len, \
         &(metrics.maxChainLength));
   metrics.avgChainLength = (float)sum_len / metrics.numberOfChains;

   return metrics;
}
