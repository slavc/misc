#ifndef UTIL_H
#define UTIL_H

#define NELEMS(array) (sizeof(array)/sizeof(array[0]))

void        *xmalloc(size_t size);              /* x{{re,m,c}alloc}, abort if operation fails */
void        *xrealloc(void *ptr, size_t size);
void        *xcalloc(size_t nmemb, size_t size);
char        *str_glue(const char *s, ...);      /* concatenate strings */
char        *ascend(const char *path_str);      /* return path one level up from path_str */
const char  *get_home_dir(void);
const char  *slash(const char *path_so_far);    /* return "/" if path_so_far doesn't end with /, otherwise "" */

#endif

