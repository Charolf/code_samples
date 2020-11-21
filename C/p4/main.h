#ifndef MAIN_H
#define MAIN_H

/* 
 * task = 0: stdin, no -n
 * task = 1:  file, no -n
 * task = 2: stdin, w/ -n
 * task = 3:  file, w/ -n
 */
#define TRUE 1
#define FALSE 0
#define DEFAULT 10

unsigned hash(const void *data);
static int compareData(const void *a, const void *b);
FILE* fileOpen(const char *fname);
void alloc_exit(void *ptr);
void print_usage();
void check_arg_helper(int argc, char *argv[], int *num_line, int *flg_count);
int check_arg(int argc, char* argv[], int *num_line);
void freeWord(const void *data);
void create_add_free(Byte *word, unsigned length, void *ht, int hasPrintable);
void open_read_helper(FILE *file, Byte **word, unsigned *wordLength, \
   int *hasPrintable, void *ht);
void open_files(int argc, char *argv[], void *ht);
void read_stdin(int argc, char *argv[], void *ht);
void print_each_helper(HTEntry *entries, int i);
void print_each(HTEntry *entries, int num_line, unsigned size);
int main(int argc, char *argv[]);

#endif
