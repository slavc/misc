/*
 * Rename files so they are compatible with FAT32.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int	 dflag; /* dry run */
int	 rflag; /* recursive */

static void
xlstat(const char *path, struct stat *p)
{
	if (lstat(path, p) == -1)
		err(1, "stat: %s", path);
}

static void *
xmalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL)
		err(1, "malloc");
	return p;
}

static DIR *
xopendir(const char *path)
{
	DIR	*p;

	p = opendir(path);
	if (p == NULL)
		err(1, "opendir: %s", path);
	return p;
}

static char *
hex(int ch, char *p)
{
	static const char	 tab[] = "0123456789ABCDEF";

	*p++ = tab[(ch >> 4) & 0xF];
	*p++ = tab[ch & 0xF];
	return p;
}

/* replace "bad" characters with their hex codes;
 * return non-zero if anything was replaced 
 */
static char *
replacebad(const char *s)
{
	const char	*rp;	/* read pointer */
	char		*wp;	/* write pointer */
	int		 nbad;	/* how many "bad" characters are in s */
	char		*buf;	/* buffer where modified string is written */

#define ISBAD(ch) \
	!isascii(ch) || (unsigned) (ch) > 127 || strchr("\\:<>?\"", (ch)) != NULL

	for (rp = s, nbad = 0; *rp != '\0'; ++rp) {
		if (ISBAD(*rp))
			++nbad;
	}
	if (nbad == 0)
		return NULL;

	/* replace each bad char with it's hex code, e.g. "AF" */
	buf = xmalloc(strlen(s) + nbad + 1);
	strcpy(buf, s);
	for (rp = s, wp = buf; *rp != '\0'; ++rp) {
		if (ISBAD(*rp)) {
			wp = hex(*rp, wp);
		} else {
			*wp++ = *rp;
		}
	}
	*wp = '\0';
	return buf;
}

static void
move(const char *oldpath, const char *newpath)
{
	if (dflag) {
		printf("mv '%s' '%s'\n", oldpath, newpath);
		return;
	}
	if (rename(oldpath, newpath) == -1)
		err(1, "rename");
	/* TODO implement actual moving */
}

/* apply func to each filename in directory pointed to by path */
static void
apply(const char *path, void (*func)(const char *))
{
	DIR		*d;
	struct dirent	*e;
	char		*newpath;

	d = xopendir(path);
	while ((e = readdir(d)) != NULL) {
		if (e->d_type == DT_DIR
		    && (!strcmp(".", e->d_name) || !strcmp("..", e->d_name))) {
			continue;
		}
		/* +2 for '\0' and a '/' */
		newpath = xmalloc(strlen(path) + strlen(e->d_name) + 2);
		sprintf(newpath, "%s/%s", path, e->d_name);
		func(newpath);
		free(newpath);
	}
	closedir(d);
}

static void
renamefat32compat(const char *path)
{
	struct stat	 st;
	char		*newpath;

	xlstat(path, &st);
	if (S_ISLNK(st.st_mode))
		return;
	newpath = replacebad(path);
	if (newpath != NULL) {
		move(path, newpath);
		if (!dflag)
			path = newpath;
	}
	if (rflag && S_ISDIR(st.st_mode))
		apply(path, renamefat32compat);
	if (newpath != NULL)
		free(newpath);
}

static void
usage(void)
{
	printf(
	    "usage: renamefat32compat [-d] [-r] <path>...\n"
	    "Rename directories and files so that they are compatible with FAT32.\n"
	    "	-d -- dry run, show what the program would do\n"
	    "	-r -- recursively descend into directories\n");
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	while ((ch = getopt(argc, argv, "dhr")) != -1) {
		switch (ch) {
		case 'd':
			++dflag;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'r':
			++rflag;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		usage();
	} else {
		while (argc--)
			renamefat32compat(*argv++);
	}

	return 0;
}
