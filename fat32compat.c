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
int	 fflag; /* force */
int	 rflag; /* recursive */

static void
xstat(const char *path, struct stat *p)
{
	if (stat(path, p) == -1)
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

/* replace bad characters with a single underscore */
static void
replacebad(char *s)
{
	char		*readp;
	char		*writep;
	char		 prev = '\0';
	const char	 subst = '_';

	for (readp = writep = s; *readp != '\0'; ++readp) {
		if (!isascii(*readp)
		    || (unsigned) *readp > 127
		    || strchr("\\:<>?\"", *readp) != NULL) {
			if (prev != subst)
				*writep++ = prev = subst;
		} else {
			*writep++ = prev = *readp;
		}
	}
	*writep = '\0';
}

static void
move(const char *oldpath, const char *newpath)
{
	printf("mv '%s' '%s'\n", oldpath, newpath);
	/* TODO implement actual moving */
}

static void
fat32compat(const char *path)
{
	DIR		*d;
	struct dirent	*e;
	struct stat	 st;
	char		*newpath;

	xstat(path, &st);
	if (rflag && S_ISDIR(st.st_mode)) {
		d = xopendir(path);
		while ((e = readdir(d)) != NULL) {
			if (e->d_type == DT_DIR
			    && (!strcmp(".", e->d_name) || !strcmp("..", e->d_name))) {
				continue;
			}
			/* +2 for '\0' and a '/' */
			newpath = xmalloc(strlen(path) + strlen(e->d_name) + 2);
			sprintf(newpath, "%s/%s", path, e->d_name);
			fat32compat(newpath);
			free(newpath);
		}
		closedir(d);
	}
	newpath = xmalloc(strlen(path) + 1);
	strcpy(newpath, path);
	replacebad(newpath);
	if (strcmp(newpath, path) != 0)
		move(path, newpath);
	free(newpath);
}

static void
usage(void)
{
	printf(
	    "usage: fat32compat [-d] [-f] [-r] <paths...>\n"
	    "	-d -- dry run, show what the program would do\n"
	    "	-f -- \"force\", more resilience towards errors; overwrite existing files\n"
	    "	-r -- recursively descend into directories\n");
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	while ((ch = getopt(argc, argv, "dfhr")) != -1) {
		switch (ch) {
		case 'd':
			++dflag;
			break;
		case 'f':
			++fflag;
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

	while (argc--)
		fat32compat(*argv++);

	return 0;
}
