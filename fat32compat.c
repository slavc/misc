/*
 * Make strings given as arguments usable as FAT32 file names.
 */
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void
fat32compat(char *s)
{
	char		*readp;
	char		*writep;
	char		 prev = '\0';
	const char	 subst = '_';

	for (readp = writep = s; *readp != '\0'; ++readp) {
		if (!isascii(*readp)
		    || (unsigned) *readp > 127
		    || strchr("\\/:<>?\"", *readp) != NULL) {
			if (prev != subst)
				*writep++ = prev = subst;
		} else {
			*writep++ = prev = *readp;
		}
	}
	*writep = '\0';
}

int
main(int argc, char **argv)
{
	while (++argv, --argc) {
		fat32compat(*argv);
		puts(*argv);
	}
	return 0;
}
