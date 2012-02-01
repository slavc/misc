#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <unistd.h>
#include <err.h>

const char *
get_home_dir(void) {
    const char *home_dir;

    if ((home_dir = getenv("HOME")) == NULL) {
        warnx("HOME not set, using / as home dir");
        home_dir = "/";
    }
}

void *
xmalloc(size_t size) {
    void *p;

    if ((p = malloc(size)) == NULL) {
        warn("malloc");
        abort();
    }

    return p;
}

void *
xrealloc(void *ptr, size_t size) {
    void *p;

    if ((p = realloc(ptr, size)) == NULL) {
        warn("realloc");
        abort();
    }
    
    return p;
}

void *
xcalloc(size_t nmemb, size_t size) {
    void *p;

    if ((p = calloc(nmemb, size)) == NULL) {
        warn("calloc");
        abort();
    }

    return p;
}

char *
str_glue(const char *s, ...) {
    char *buf = NULL;
    size_t bufsize = 0, len = 0;
    va_list ap;

    assert(s);

    va_start(ap, s);
    do {
        len = strlen(s);
        buf = xrealloc(buf, bufsize + len);
        memcpy(buf + bufsize, s, len);
        bufsize += len;
    } while (s = va_arg(ap, const char *));
    va_end(ap);

    buf = xrealloc(buf, bufsize + 1);
    buf[bufsize] = '\0';

    return buf;
}

char *
ascend(const char *path_str) {
    char *path,
         *last_slash;

    path = strdup(path_str);
    last_slash = strrchr(path, '/');
        
    if (last_slash != path) {
        *last_slash = '\0';
        /* XXX resize the str? */
    } else
        *(last_slash + 1) = '\0';

    return path;
}

const char *
slash(const char *path_so_far) {
    size_t len;
    const char *last_char;

    assert(path_so_far);

    last_char = strrchr(path_so_far, '\0');
    if (last_char != path_so_far)
        --last_char;

    if (*last_char == '/')
        return "";
    else
        return "/";
}
