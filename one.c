
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "one.h"
#include "prs.h"


int one_list(const char *archname) {
	char *buf = file_to_buffer(archname, NULL);
	ONESSR_HEAD *opre = (void*)buf;
	ONESSR_INODE *otoc;
	int i;

	if(!buf) { return err("one_list: couldn't load file", -1); }
	one_fix_endians(buf);
	if(opre->toc_offset != ONESSR_TOCOFS) {
		return err("one_list: invalid toc offset", -1);
	}

	otoc = (void*)(buf + opre->toc_offset);
	for(i = 0; i < opre->file_count; ++i) {
		puts(otoc[i].fname);
	}

	free(buf);
	return 0;
}

/* CREATION-RELATED */
int one_create(const char *archname, int fcount, char **flist) {
	int tocsize = fcount * sizeof(ONESSR_INODE);
	ONESSR_HEAD opre = { fcount, ONESSR_TOCOFS, tocsize+ONESSR_TOCOFS, 0 };
	ONESSR_INODE *otoc = malloc(tocsize);
	int i; FILE *f;
	int *cmpsizes = malloc(fcount * sizeof(int));
	char **fbuf = malloc(fcount * sizeof(char*));
	int cur_pos = opre.data_offset;

	if((f = fopen(archname, "rb"))) {
		free(fbuf); free(cmpsizes); free(otoc); fclose(f);
		return err("one_create: archive already exists", -1);
	}

	for(i = 0; i < fcount; ++i) {
		fbuf[i] = one_create_inode(flist[i], &(otoc[i]), &cur_pos);
		otoc[i].id = i;
		cmpsizes[i] = otoc[i].size_cmp;
		one_fix_endians_inode(&(otoc[i]));
	}
	one_fix_endians_head(&opre);

	f = fopen(archname, "wb");
	if(!f) {
		for(i = 0; i < fcount; ++i) { free(fbuf[i]); }
		free(fbuf); free(cmpsizes); free(otoc);
		return err("one_create: couldn't write archive file", -1);
	}
	fwrite(&opre, sizeof(ONESSR_HEAD), 1, f);
	fwrite(otoc, sizeof(ONESSR_INODE), fcount, f);
	free(otoc);
	for(i = 0; i < fcount; ++i) {
		fwrite(fbuf[i], 1, cmpsizes[i], f);
		free(fbuf[i]);
	}
	free(fbuf); free(cmpsizes);
	return 0;
}

char *one_create_inode(const char *fname, ONESSR_INODE *otoc, int *cur_pos) {
	char *srcbuf, *destbuf;

	strncpy(otoc->fname, fname, ONESSR_MAXNAME);

	srcbuf = file_to_buffer(fname, &(otoc->size_dec));
	destbuf = malloc(otoc->size_dec);
	otoc->size_cmp = prs_compress(srcbuf, destbuf, otoc->size_dec);
	free(srcbuf);

	otoc->offset = *cur_pos;
	*cur_pos += otoc->size_cmp;

	return destbuf;
}

/* EXTRACTION-RELATED */
int one_extract(const char *archname) {
	int ret;
	char *buf = file_to_buffer(archname, NULL);
	if(!buf) { return err("one_extract: couldn't load file", -1); }
	one_fix_endians(buf);
	ret = one_extract_all(buf);
	free(buf);
	if(ret != 0) { return err("one_extract: error extracting", ret); }
	return 0;
}

int one_extract_all(char *buf) {
	ONESSR_HEAD *opre = (void*)buf;
	ONESSR_INODE *otoc;
	int i;

	if(!opre) { return err("one_extract_all: null argument", -1); }
	if(opre->toc_offset != ONESSR_TOCOFS) {
		return err("one_extract_all: invalid toc offset", -1);
	}

	otoc = (void*)(buf + opre->toc_offset);
	for(i = 0; i < opre->file_count; ++i) {
		if(one_extract_inode(buf, &(otoc[i])) < 0) {
			return err("one_extract_all: error extracting", i+1);
		}
	}
	return 0;
}

int one_extract_inode(char *buf, ONESSR_INODE *otoc) {
	char *srcptr = buf + otoc->offset;
	int dsize = otoc->size_dec;
	char *fname = otoc->fname;
	char *prsbuf;

	if(dsize != prs_decompress_size(srcptr)) {
		return err("one_extract_inode: decompressed size mismatch", -1);
	}

	prsbuf = malloc(dsize);
	prs_decompress(srcptr, prsbuf);
	/*puts(fname);*/
	if(buffer_to_file(prsbuf, dsize, fname) < 0) {
		return err("one_extract_inode: error occurred writing", -1);
	}
	free(prsbuf);
	return 0;
}



/* ENDIAN COMPATIBILITY */
int one_fix_endians(char *buf) {
	ONESSR_HEAD *opre = (void*)buf;
	ONESSR_INODE *otoc;
	int i;

	if(!opre) { return err("one_fix_endians: null argument", -1); }

	one_fix_endians_head(opre);
	if(opre->toc_offset != ONESSR_TOCOFS) {
		return err("one_fix_endians: invalid toc offset", -1);
	}

	otoc = (void*)(buf + opre->toc_offset);
	for(i = 0; i < opre->file_count; ++i) {
		one_fix_endians_inode(&(otoc[i]));
	}
	return 0;
}

int one_fix_endians_head(ONESSR_HEAD *opre) {
	fix_endian_32bit(&(opre->file_count));
	fix_endian_32bit(&(opre->toc_offset));
	fix_endian_32bit(&(opre->data_offset));
	return 0;
}

int one_fix_endians_inode(ONESSR_INODE *otoc) {
	fix_endian_32bit(&(otoc->id));
	fix_endian_32bit(&(otoc->offset));
	fix_endian_32bit(&(otoc->size_cmp));
	fix_endian_32bit(&(otoc->size_dec));
	return 0;
}

void fix_endian_32bit(int *x) {
	static char _endianness = 0;
	if(_endianness == 0) {
		short a = 0x1234;
		char *b = (void*)&a;
		_endianness = (*b == 0x12) ? 'B' : 'L';
	}

	if(_endianness == 'L') {
		*x = (((*x)&0xFF)<<24)
		   | (((*x)&0xFF00)<<8)
		   | (((*x)&0xFF0000)>>8)
		   | (((*x)&0xFF000000)>>24);
	}
}



/* FILE OPERATIONS */
char* file_to_buffer(const char *fname, int *fsize) {
	int size;
	char* buf = NULL;
	FILE *f = fopen(fname, "rb");
	if(!f) {
		err("file_to_buffer: couldn't open file to read", 0);
		return NULL;
	}

	fseek(f, 0, SEEK_END); size = ftell(f); rewind(f);
	buf = malloc(size);
	if(!buf) {
		err("file_to_buffer: not enough memory", 0);
		fclose(f); return NULL;
	}
	fread(buf, 1, size, f);
	if(ferror(f)) {
		perror("buffer_to_file");
		free(buf); fclose(f); return NULL;
	}
	fclose(f);
	if(fsize) { *fsize = size; }
	return buf;
}

int buffer_to_file(const char *buf, int size, const char* fname) {
	FILE *f = fopen(fname, "wb");
	if(!f) { return err("buffer_to_file: can't open file to write", -1); }

	fwrite(buf, 1, size, f);
	if(ferror(f)) {
		fclose(f);
		perror("buffer_to_file");
		return -1;
	}

	fclose(f);
	return 0;
}

/* ERROR MESSAGE */
int err(const char *s, int r) {
	fprintf(stderr, "%s\n", s);
	return r;
}
