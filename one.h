#ifndef __ONE_H__
#define __ONE_H__

/* note that all integers in this format are big endian, like the wii.  the
 * program will detect endianness of the host system and swap accordingly.
 */

typedef struct onessr_head {
	int file_count; /* number of files stored in the .one */
	int toc_offset; /* where the toc starts, usually 0x10 = 16 */
	int data_offset; /* location of the end of the toc/beginning of data */
	int reserved; /* probably just zero */
} ONESSR_HEAD;

typedef struct onessr_inode {
	char fname[32]; /* name of the stored file */
	int id; /* index of the file (0,1,2,3,...,file_count-1) */
	int offset; /* location of the data in the .one */
	int size_cmp; /* size as it is stored in the .one */
	int size_dec; /* presumably the decompressed size */
} ONESSR_INODE;

#define ONESSR_TOCOFS 16
#define ONESSR_MAXNAME 32

int one_list(const char *archname);

int one_create(const char *archname, int fcount, char **flist);
char *one_create_inode(const char *fname, ONESSR_INODE *otoc, int *cur_pos);

int one_extract(const char *archname);
int one_extract_all(char *buf);
int one_extract_inode(char *buf, ONESSR_INODE *otoc);

char* file_to_buffer(const char *fname, int *fsize);
int buffer_to_file(const char *buf, int size, const char* fname);

int one_fix_endians(char *buf);
int one_fix_endians_head(ONESSR_HEAD *opre);
int one_fix_endians_inode(ONESSR_INODE *otoc);
void fix_endian_32bit(int *x);

int err(const char *s, int r);

#endif /* __ONE_H__ */
