/*
 * Copyright (c) 2009 Sviatoslav Chagaev <slava@zb.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARGS 255
#define FS " \t"
#define SOFTQUOT '\"'
#define PROMPTFMT "%s:%s$ "
#define MAXPROMPT (((FILENAME_MAX * 2) < 1024) ? 1024 : (FILENAME_MAX * 2))

#define INVALID_INDEX (~((size_t) 0))

/* syntax tree type */
struct tree {
	int             type;
	char           *s;
	struct tree    *left;
	struct tree    *right;
};
typedef struct tree tree_t;
enum tree_types {		/* in order of precedance */
	T_SEMICOLON,
	T_AND,
	T_OR,
	T_NOT,
	T_BG,
	T_PIPE,
	T_SUPPRESS_OUTPUT,
	T_REDIR_STDIN,
	T_REDIR_STDOUT_APPEND,
	T_REDIR_STDERR,
	T_REDIR_STDOUT,
	T_EVAL
};
char           *keywords[] = {
	";",
	"&&",
	"||",
	"!",
	"&",
	"|",
	"><",
	"<",
	">>",
	"^>",
	">",
	NULL
};
char           *pkeywords[] = {	/* for print_tree() */
	";",
	"AND",
	"OR",
	"NOT",
	"BG",
	"PIPE",
	"SUPP",
	"R_IN",
	"R_OUT_A",
	"R_ERR",
	"R_OUT",
	"EVAL"
};

typedef struct var {
	char *name;
	int type;
	struct {
		char *string;
		long integer;
	} val;
} var_t;
enum var_types {
	STRING,
	INTEGER
};

typedef struct var_tab{
	var_t **v;
	size_t n;
	size_t threshold;
	size_t size;
} var_tab_t;

enum exec_modes {
	FG,
	BG
};

char           *username = NULL;
char            currentworkdir[FILENAME_MAX] = "";
int             syntax_error = 0;
char           *builtin_commands[] = {
	"cd",
	"export",
	"unset",
	NULL
};
enum builtin_commands {
	CMD_CD,
	CMD_EXPORT,
	CMD_UNSET,
	CMD_NOT_BUILT_IN
};

int  eval(char *buf, int exec_mode);
char *stripspace(char *s);
void resize_var_tab(var_tab_t *tab, size_t size);
void delete_from_var_tab(var_tab_t *tab, char *name, size_t index);
void delete_var(var_t *v);

size_t
hash(const char *s) {
	size_t h = 0;

	while (*s != '\0') {
		h += *(s++);
		h *= 31;
	}

	return h;
}

void
init_var_tab(var_tab_t *vt) {
	memset(vt, 0, sizeof(var_tab_t));
}

int
place_in_var_tab(var_tab_t *tab, const var_t *var, size_t from, size_t to) {
	size_t i;

	for (i = from; i < to; ++i) {
		if (tab->v[i] == NULL) {
			tab->v[i] = (var_t *) var;
			++tab->n;
			return 1;
		}
		if (!strcmp(tab->v[i]->name, var->name)) {
			delete_var(tab->v[i]);
			tab->v[i] = (var_t *) var;
			++tab->n;
			return 1;
		}
	}

	return 0;
}

void
add_to_var_tab(var_tab_t *tab, const var_t *var) {
	size_t tabsize, tabn, tabthreshold;
	size_t h;
	size_t i;

	if (tab->size == 0) {
		tab->size = 2;
		tab->v = calloc(2, sizeof(var_t *));
		tab->threshold = 2;
	} else if (tab->n >= tab->threshold) {
		resize_var_tab(tab, tab->size * 2);
	}

	h = hash(var->name) % tab->size;

	if (tab->v[h] == NULL) {
		tab->v[h] = (var_t *) var;
		++tab->n;
		return;
	}

	if (!place_in_var_tab(tab, var, h, tab->size))
		place_in_var_tab(tab, var, 0, h);
}

size_t
find_in_var_tab(var_tab_t *tab, const char *name, size_t from, size_t to) {
	size_t i;

	for (i = from; i < to; ++i) {
		if (tab->v[i] == NULL)
			return INVALID_INDEX;
		if (!strcmp(name, tab->v[i]->name))
			return i;
	}
}

/* return index in the array of pointers to vars of a var */
size_t
index_var_tab(var_tab_t *tab, const char *name) {
	size_t i, h;

	if (tab->size == 0 || tab->n == 0)
		return INVALID_INDEX;
	h = hash(name) % tab->size;
	if (tab->v[h] == NULL)
		return INVALID_INDEX;
	if ((i = find_in_var_tab(tab, name, h, tab->size)) == INVALID_INDEX)
		return find_in_var_tab(tab, name, 0, h);
	return i;
}

var_t *
lookup_var_tab(var_tab_t *tab, const char *name) {
	size_t i;

	if ((i = index_var_tab(tab, name)) == INVALID_INDEX)
		return NULL;
	return tab->v[i];
}

/* if name == NULL, index is used */
void
delete_from_var_tab(var_tab_t *tab, char *name, size_t index) {
	size_t i;

	if (name) {
		if ((i = index_var_tab(tab, name)) == INVALID_INDEX)
			return;
	} else {
		i = index;
		if (i >= tab->size)
			return;
	}

	if (!tab->v[i])
		return;

	delete_var(tab->v[i]);
	tab->v[i] = NULL;
	if (--tab->n < tab->size/2)
		resize_var_tab(tab, tab->size/2);
	else
		resize_var_tab(tab, tab->size);
}

/* call with current tab's size to rehash only */
void
resize_var_tab(var_tab_t *tab, size_t size) {
	size_t i;
	var_tab_t new_tab;

	init_var_tab(&new_tab);

	new_tab.v = calloc(size, sizeof(var_t *));
	new_tab.size = size;
	new_tab.threshold = size * 0.95;
	new_tab.n = 0;

	printf("resize_var_tab(): size = %lu; threshold = %lu;\n", new_tab.size, new_tab.threshold);

	for (i = 0; i < tab->size; ++i) {
		if (!tab->v[i])
			continue;
		add_to_var_tab(&new_tab, tab->v[i]);
	}
	free(tab->v);
	*tab = new_tab;
}

var_t *
new_var(const char *name, const char *string, long integer) {
	var_t *v = calloc(1, sizeof(var_t));
	enum { bufsiz = 128 };
	char buf[bufsiz] = "";

	v->name = strdup(name);
	if (string) {
		v->val.string = strdup(string);
		v->val.integer = atol(string);
	} else {
		snprintf(buf, bufsiz, "%l", integer);
		v->val.string = strdup(buf);
		v->val.integer = integer;
	}

	return v;
}

void
delete_var(var_t *v) {
	if (v == NULL)
		return;

	free(v->val.string);
	free(v);
}

tree_t         *
new_tree(void)
{
	tree_t         *t;

	t = calloc(1, sizeof(tree_t));

	return t;
}

char           *
chunkdup(const char *p1, const char *p2)
{
	char           *buf;
	size_t          len;

	len = p2 - p1;
	buf = malloc(len + 1);
	memcpy(buf, p1, len);
	buf[len] = '\0';

	return buf;
}

/* Print syntax error msg */
void
psyntaxerr(const char *msg)
{
	warnx("syntax error: %s", msg);
	syntax_error = 1;
}

char           *
find_redir_end(const char *s, char **kws)
{
	int             i;
	char           *p, *rp = NULL;

	for (i = 0; kws[i] != NULL; ++i) {
		if (rp == NULL || (p = strstr(s, kws[i])) < rp)
			rp = p;
	}

	if (rp == NULL)
		return (char *) s + strlen(s);
	else
		return rp;
}

tree_t         *
build_tree(char *s)
{
	int             i;
	char           *p, *q, *kw, *chunk;
	size_t          slen, kwlen;
	tree_t         *t;

	t = new_tree();

	for (i = 0; (kw = keywords[i]) != NULL; ++i) {

		printf("build_tree: searching \"%s\" in \"%s\"\n", kw, s);

		if ((p = strstr(s, kw)) == NULL)
			continue;

		t->type = i;

		switch (i) {
		case T_NOT:
			if (*(p + 1) == '\0') {
				psyntaxerr("operator `!' with no argument");
				free(t);
				return NULL;
			}
			slen = strlen(s);
			kwlen = strlen(kw);
			chunk = chunkdup(p + kwlen, s + slen);
			t->left = build_tree(chunk);
			free(chunk);
			break;
		case T_SUPPRESS_OUTPUT:
			chunk = chunkdup(s, p);
			t->left = build_tree(chunk);
			free(chunk);
			break;
		case T_REDIR_STDIN:
		case T_REDIR_STDOUT_APPEND:
		case T_REDIR_STDOUT:
		case T_REDIR_STDERR:
			kwlen = strlen(kw);
			q = find_redir_end(p + kwlen, keywords);
			chunk = chunkdup(p + kwlen, q);
			t->left = build_tree(chunk);
			free(chunk);

			chunk = chunkdup(s, p);
			t->right = build_tree(chunk);
			free(chunk);
			break;
		default:	/* ordinary, binary operators */
			chunk = chunkdup(s, p);
			t->left = build_tree(chunk);
			free(chunk);

			slen = strlen(s);
			kwlen = strlen(kw);
			chunk = chunkdup(p + kwlen, s + slen);
			t->right = build_tree(chunk);
			free(chunk);
			break;
		}

		return t;
	}

	/* no keywords met -- terminator */
	if (*s == '\0')
		return NULL;
	t->type = T_EVAL;
	t->s = strdup(s);

	return t;
}

void
delete_tree(tree_t *t) {
	if (t == NULL)
		return;

	delete_tree(t->left);
	delete_tree(t->right);
	free(t->s);
	free(t);
}
	

int
trav_tree(tree_t * t, int exec_mode)
{
	int             rc;
	int fd;

	printf("trav_tree(): pid=%d\n", getpid());

	if (t == NULL)
		return 0;

	switch (t->type) {
	case T_EVAL:
		rc = eval(t->s, exec_mode);
		return rc;
		break;
	case T_PIPE:
	case T_SUPPRESS_OUTPUT:
		fd = open("/dev/null", O_WRONLY, 0);
		dup2(fd, STDOUT_FILENO);
		return trav_tree(t->right, FG);
		break;
	case T_REDIR_STDIN:
		if ((fd = open(stripspace(t->left->s), O_RDONLY, 0666)) < 0) {
			warn("\"%s\"", t->left->s);
			break;
		}
		dup2(fd, STDIN_FILENO);
		return trav_tree(t->right, FG);
		break;
	case T_REDIR_STDOUT_APPEND:
		if ((fd = open(stripspace(t->left->s), O_WRONLY | O_APPEND | O_CREAT, 0666)) < 0) {
			warn("\"%s\"", t->left->s);
			break;
		}
		dup2(fd, STDOUT_FILENO);
		return trav_tree(t->right, FG);
		break;
	case T_REDIR_STDOUT:
		if ((fd = open(stripspace(t->left->s), O_WRONLY | O_TRUNC | O_CREAT, 0666)) < 0) {
			warn("\"%s\"", t->left->s);
			break;
		}
		dup2(fd, STDOUT_FILENO);
		return trav_tree(t->right, FG);
		break;
	case T_REDIR_STDERR:
		if ((fd = open(stripspace(t->left->s), O_WRONLY | O_TRUNC | O_CREAT, 0666)) < 0) {
			warn("\"%s\"", t->left->s);
			break;
		}
		dup2(fd, STDERR_FILENO);
		return trav_tree(t->right, FG);
		break;
	case T_SEMICOLON:
		trav_tree(t->left, FG);
		return trav_tree(t->right, FG);
		break;
	case T_AND:
		if ((rc = trav_tree(t->left, FG)) == 0)
			return trav_tree(t->right, FG);
		else
			return rc;
		break;
	case T_OR:
		if ((rc = trav_tree(t->left, FG)) != 0)
			return trav_tree(t->right, FG);
		else
			return rc;
		break;
	case T_NOT:
		return !trav_tree(t->left, FG);
		break;
	case T_BG:
		return trav_tree(t->left, BG);
		break;
	}

	return 0;
}

void
print_tree(tree_t * t)
{
	if (t == NULL)
		return;

	printf("%s ", pkeywords[t->type]);
	if (t->type == T_EVAL)
		printf("%s ", t->s);
	print_tree(t->left);
	print_tree(t->right);
}

int
isdelim(int c, const char *delim)
{
	while (*delim != '\0') {
		if (c == *delim)
			return c;
		++delim;
	}

	return 0;
}

char           *
token(char **strp, const char *delim)
{
	char           *s, *p;

	if (strp == NULL || *strp == NULL)
		return NULL;

	p = s = *strp;
	/* len = strlen(s); */

	if (*s == SOFTQUOT) {
		p = ++s;
		while (*p != '\0') {
			if (*p == SOFTQUOT) {
				if (p == s || *(p - 1) != '\\') {
					*(p++) = '\0';
					while (isdelim(*p, delim))
						++p;
					*strp = ((*p == '\0') ? (NULL) : (p));
					return s;
				} else {
					memmove(p - 1, p, strlen(p) + 1);
				}
			} else {
				++p;
			}
		}
		*strp = NULL;
		return s;
	} else {
		while (*p != '\0') {
			if (isdelim(*p, delim)) {
				if (p == s || *(p - 1) != '\\') {
					*(p++) = '\0';
					while (isdelim(*p, delim))
						++p;
					*strp = ((*p == '\0') ? (NULL) : (p));
					return s;
				} else {
					memmove(p - 1, p, strlen(p) + 1);
				}
			} else {
				++p;
			}
		}
		*strp = NULL;
		return s;
	}
}

char           *
stripspace(char *s)
{
	char           *p;
	size_t          len;

	assert(s != NULL);

	while (isspace(*s))
		++s;

	len = strlen(s);
	p = s + len - 1;

	while (p > s && isspace(*p)) {
		*p = '\0';
		--p;
	}

	return s;
}

int
mkargv(char *cmdline, char **argv, int maxargs)
{
	int             i = 0;
	char           *p;

	while (i < maxargs && (p = token(&cmdline, FS)) != NULL)
		argv[i++] = p;
	argv[i] = NULL;

	return i;
}

char           *
prompt(void)
{
	static int      need_update;
	static char     buf[MAXPROMPT];

	if (*currentworkdir == '\0') {
		getcwd(currentworkdir, FILENAME_MAX);
		need_update = 1;
	}
	if (username == NULL) {
		username = getlogin();
		need_update = 1;
	}
	if (need_update) {
		snprintf(buf, MAXPROMPT, PROMPTFMT, username, currentworkdir);
		need_update = 1;
	}
	return buf;
}

int
isbuiltin(char *cmd)
{
	int             i;

	for (i = 0; builtin_commands[i] != NULL && strcmp(builtin_commands[i], cmd); ++i)
		 /* empty */ ;
	return i;
}

int
exec_cmd(int argc, char **argv, int exec_mode)
{
	int             status;
	pid_t           pid;

	pid = fork();

	if (pid == -1) {
		warn("fork");
	} else if (pid == 0) {
		if (execvp(argv[0], argv))
			warn("execvp");
	} else {
		if (exec_mode == BG)
			return 0;
		if (wait(&status) == -1)
			warn("waitpid");
		return status;
	}

	return status;
}

int
exec_builtin(int argc, char **argv, int cmdi)
{
	int             rc;

	switch (cmdi) {
	case CMD_CD:
		if ((rc = chdir(argv[1])))
			warn("chdir");
		else
			currentworkdir[0] = '\0';
		break;
	case CMD_EXPORT:
		if ((rc = putenv(argv[1])))
			warn("putenv");
		break;
	case CMD_UNSET:
		unsetenv(argv[1]);
		rc = 0;
		break;
	default:
		rc = -1;
	}

	return rc;
}

int
eval(char *buf, int exec_mode)
{
	int             i;
	int             status;
	int             argc;
	char           *argv[MAXARGS + 1];
	char           *sbuf;

	sbuf = stripspace(buf);

	argc = mkargv(sbuf, argv, MAXARGS);

	i = isbuiltin(argv[0]);

	if (i == CMD_NOT_BUILT_IN) {
		status = exec_cmd(argc, argv, exec_mode);
	} else {
		status = exec_builtin(argc, argv, i);
	}

	return status;
}

void
save_std_fds(int *in, int *out, int *err) {
	*in = dup(STDIN_FILENO);
	*out = dup(STDOUT_FILENO);
	*err = dup(STDERR_FILENO);
}

void
restore_std_fds(int in, int out, int err) {
	dup2(in, STDIN_FILENO);
	dup2(out, STDOUT_FILENO);
	dup2(err, STDERR_FILENO);
}

#ifndef TEST
int
main(int argc, char **argv)
{
	char           *line;
	char           *cline;
	char           *p;
	pid_t pid;
	int rc;
	tree_t         *t;
	int stdinfd, stdoutfd, stderrfd;

	while ((line = readline(prompt())) != NULL) {
		cline = strdup(line);
		p = stripspace(cline);

		t = build_tree(p);
		print_tree(t);

		save_std_fds(&stdinfd, &stdoutfd, &stderrfd);

		/*
		pid = fork();
		if (pid < 0) {
			warn("fork");
		} else if (pid > 0) {
			wait(&rc);
		} else {
		*/
			trav_tree(t, FG);
		/*
		}
		*/
		restore_std_fds(stdinfd, stdoutfd, stderrfd);

		delete_tree(t);
		// free(t);

		printf("\n");

		free(cline);
		free(line);
	}
	printf("EOF, exiting\n");

	exit(0);
}
#endif

#if defined(TEST) && defined(TEST_VAR_TAB)

void
print_var_tab(var_tab_t *vt) {
	size_t i;
	printf("var_tab: n = %lu; size = %lu; threshold = %lu;\n", vt->n, vt->size, vt->threshold);

	if (!vt->n)
		return;
	for (i = 0; i < vt->size; ++i) {
		printf("[%3lu]", i);
		if (vt->v[i])
			printf(": \"%s\":\"%s (%l)\n", vt->v[i]->name, vt->v[i]->val.string, vt->v[i]->val.integer);
		else
			putchar('\n');
	}
}

int
main(int argc, char **argv) {
	enum { bufsiz = 1024 };
	char buf[bufsiz] = "";
	var_tab_t var_tab;
	size_t len;
	char *name, *val, *p;
	var_t *var;

	/* init_var_tab(&var_tab); */
	bzero(&var_tab, sizeof(var_tab_t));

	for ( ; ; ) {
		printf("> ");

		if (fgets(buf, bufsiz, stdin) == NULL)
			exit(0);
		stripspace(buf);

		if (!strcmp(buf, "quit"))
			exit(0);

		if (!strcmp(buf, "p")) {
			print_var_tab(&var_tab);
			continue;
		}

		if ((p = strchr(buf, '=')) == NULL)
			continue;
		*p = '\0';
		name = buf;
		val = p+1;

		var = new_var(name, val, 0);
		printf("adding (\"%s\":\"%s\")\n", var->name, var->val.string);
		add_to_var_tab(&var_tab, var);
		printf("ok\n");
	}

	exit(0);
}
#endif

