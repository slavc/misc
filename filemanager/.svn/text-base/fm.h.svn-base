#ifndef FM_H
#define FM_H

#include <dirent.h>

#define FIFO         DT_FIFO
#define CHR_DEVICE   DT_CHR
#define DIRECTORY    DT_DIR
#define BLK_DEVICE   DT_BLK
#define REGULAR_FILE DT_REG
#define SYM_LINK     DT_LNK
#define SOCKET       DT_SOCK

/* file system node */
typedef struct fsnode fsnode_t;
struct fsnode {
    char            *path;
    char            *name; /* the last part of path or / */
    int              type;
    struct stat     *stat;
    fsnode_t        *children;
    size_t           nchildren;
};

int       list_dir(const char *path, fsnode_t *node);
fsnode_t *add_child(fsnode_t *node);    /* add new child to node->children */
void      clear_fsnode(fsnode_t *node); /* free all members of node, and bzero node */
void      bzero_fsnode(fsnode_t *p);
void      sort_fsnodes_children(fsnode_t *node);
int       compare_fsnodes_by_name(const void *ptr1, const void *ptr2);
int       compare_fsnodes_by_type(const void *ptr1, const void *ptr2);

#endif

