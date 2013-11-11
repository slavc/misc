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
	char		*rp; /* read pointer */
	char		*wp; /* write pointer */
	char		 prev = '\0';
	const char	 subst = '_';

	rp = wp = strrchr(s, '/');
	if (rp == NULL)
		rp = wp = s;
	while (*rp != '\0') {
		if (!isascii(*rp)
		    || (unsigned) *rp > 127
		    || strchr("\\:<>?\"", *rp) != NULL) {
			if (prev != subst)
				*wp++ = prev = subst;
		} else {
			*wp++ = prev = *rp;
		}
		++rp;
	}
	*wp = '\0';
}

static void
move(const char *oldpath, const char *newpath)
{
	printf("mv '%s' '%s'\n", oldpath, newpath);
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
fat32compat(const char *path)
{
	struct stat	 st;
	char		*newpath;

	xstat(path, &st);
	if (rflag && S_ISDIR(st.st_mode))
		apply(path, fat32compat);
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
	    "usage: fat32compat [-d] [-f] [-r] [-l] <paths...>\n"
	    "	-d -- dry run, show what the program would do\n"
	    "	-f -- \"force\", more resilience towards errors; overwrite existing files\n"
	    "	-r -- recursively descend into directories\n");
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	while ((ch = getopt(argc, argv, "dfhrl")) != -1) {
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
