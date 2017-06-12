
#include <stdio.h>
#include <stdlib.h>
#include "one.h"

/* PROTOTYPES */
int usage(void);

/* FUNCTIONS */
int main(int argc, char **argv) {
	if(argc < 3) { return usage(); }

	switch(argv[1][0]) {
		case 'l': return one_list(argv[2]);
		case 'x': return one_extract(argv[2]);
		case 'c': return one_create(argv[2], argc-3, argv+3);
		default: break;
	}

	return usage();
}

int usage(void) {
	puts(
"\nssr-one-util: program to handle archives from 'Sonic and the Secret Rings'"
"\n                *Sonic and related are (c) Sega, this program is not"
"\n                related to or affiliated with Sega in any way"
"\n"
"\n  usage: ssr-one-util [action] [filename.one] [files...]"
"\n    [action]:  l => list, x => extract, c => create."
"\n    [filename.one]: a valid .one file from 'Sonic and the Secret Rings'"
"\n    [files...]: list of one or more files to pack when creating an archive."
"\n"
"\n  credits:\n"
"\n    lifning - .one archive handling code, and this utility"
"\n    fuzziqer - .prs format (de)compression routines"
"\n"
	);
	return 0;
}
