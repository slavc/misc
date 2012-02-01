/* A CGI program which prints out it's environment. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#define NELEMS(a) (sizeof(a)/sizeof((a)[0]))

extern char **environ;

char *hidden_vars[] = {
	"SERVER_ADMIN",
	"SCRIPT_FILENAME"
};

int
starts_with(const char *big, const char *small) {
	while (*big != '\0' && *big == *small) {
		++big;
		++small;
	}
	return *small == 0;
}

int
main(int argc, char **argv)
{
	int i, j, c;

	printf("Content-Type: text/plain\r\n\r\n");

	printf("### PROGRAM ARGUMENTS ###\n\n");
	for (i = 0; i < argc; ++i)
		printf("%s\n", argv[i]);

	printf("\n\n\n### ENVIRONMENT VARIABLES ###\n\n");
	for (i = 0; environ[i] != NULL; ++i) {
		for (j = 0; j < NELEMS(hidden_vars); ++j) {
			if (starts_with(environ[i], hidden_vars[j]))
				break;
		}
		if (j < NELEMS(hidden_vars))
			continue;
		printf("%s\n", environ[i]);
	}

	printf("\n\n\n### STANDARD INPUT ###\n\n");
	while ((c = getchar()) != EOF)
		putchar(c);

	exit(0);
}

