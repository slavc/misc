#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <err.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>

struct file;

struct dir {
	const char		*path;
	struct file		*files;
	int			 nfiles;
};

struct file {
	const char		*name;
	int			 type;
	const struct stat	*st;
};

int
dir_list(struct dir *d, const char *path)
{
}

int
dir_get_nfiles(struct dir *d)
{
}

const char *
dir_file_get_name(struct dir *d, int i)
{
}

int
dir_file_get_type(struct dir *d, int i)
{
}

dir_file_get_stat(struct dir *d, int i)
{
}
