#ifndef XMEM_H
#define XMEM_H

void	*xmalloc(size_t);
void	*xcalloc(size_t, size_t);
void	*xrealloc(void *, size_t);
void	 xfree(void *);
char	*xstrdup(const char *);

#endif
