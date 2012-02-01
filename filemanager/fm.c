#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <dirent.h>

#include "fm.h"
#include "util.h"

void
bzero_fsnode(fsnode_t *p) {
    bzero(p, sizeof(*p));
}

void
clear_fsnode(fsnode_t *node) {
    size_t i;

    assert(node);

    free(node->path);
    free(node->name);
    free(node->stat);
    for (i = 0; i < node->nchildren; ++i) {
        clear_fsnode(node->children + i);
    }
    free(node->children);

    bzero_fsnode(node);
}

fsnode_t *
add_child(fsnode_t *node) {
    assert(node);

    node->children = xrealloc(node->children, (node->nchildren + 1) * sizeof(fsnode_t));
    bzero_fsnode(node->children + node->nchildren);

    return node->children + node->nchildren++;
}

int
list_dir(const char *path, fsnode_t *node) {
    DIR *dp;
    struct dirent *de;
    fsnode_t *child;

    assert(path && node);

    clear_fsnode(node);
    node->path = strdup(path);

    /* always add ".." entry so one could go up from a restricted dir */
    child = add_child(node);
    child->path = str_glue(path, slash(path), "..", NULL);
    child->name = strdup("..");
    child->type = DIRECTORY;

    if ((dp = opendir(path)) == NULL) {
        warn("opendir %s", path);
        return -1;
    }

    while (de = readdir(dp)) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        child = add_child(node);

        child->path = str_glue(path, slash(path), de->d_name, NULL);
        child->name = strdup(de->d_name);
        child->type = de->d_type;
    }
    closedir(dp);

    return 0;
}

int
compare_fsnodes_by_type(const void *ptr1, const void *ptr2) {
    const fsnode_t *node1 = ptr1, *node2 = ptr2;

    if (node1->type == node2->type)
        return 0;
    else if (node1->type == DIRECTORY)
        return -1;
    else
        return 1;
}

int
compare_fsnodes_by_name(const void *ptr1, const void *ptr2) {
    const fsnode_t *node1 = ptr1, *node2 = ptr2;

    return strcmp(node1->name, node2->name);
}

void
sort_fsnodes_children(fsnode_t *node) {
    size_t ndirs, nfiles;

    assert(node);

    if (node->nchildren < 1)
        return;

    qsort(node->children, node->nchildren, sizeof(fsnode_t), compare_fsnodes_by_type);

    for (ndirs = 0; node->children[ndirs].type == DIRECTORY && ndirs < node->nchildren; ++ndirs)
        /* empty */;
    nfiles = node->nchildren - ndirs;

    if (ndirs)
        qsort(node->children, ndirs, sizeof(fsnode_t), compare_fsnodes_by_name);
    if (nfiles)
        qsort(node->children + ndirs, nfiles, sizeof(fsnode_t), compare_fsnodes_by_name);
}

#ifdef TEST_FM
void
print_fsnode(fsnode_t *node) {
    size_t i;
    fsnode_t *child;
    assert(node);

    printf("%s\n", node->path);

    for (i = 0, child = node->children; i < node->nchildren; ++i, ++child) {
        printf("  %s (%s)\n", child->name, child->path);
    }
}

int
main(int argc, char **argv) {
    fsnode_t node;

    if (argc < 2)
        return 0;

    list_dir(argv[1], &node);
    print_fsnode(&node);

    return 0;
}
#endif
