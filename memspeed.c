/*
 * Print apparent speed of sequential read and write operations on system
 * memory.
 *
 * Compile without optimization:
 *  cc -O0 -o memspeed memspeed.c
 */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SIZE		100 /* MiB */
#define USEC_IN_SEC	1000000.0

static void
memread(uint8_t *ptr, size_t size)
{
	unsigned	 sum = 0;

	while (size--)
		sum = *ptr++;
}

static void
memwrite(uint8_t *ptr, size_t size)
{
	unsigned	 count = 0;

	while (size--)
		*ptr++ = (uint8_t)count++;
}

static double
measure(void (*func)(uint8_t *, size_t), uint8_t *ptr, size_t size)
{
	struct timeval	 t1;
	struct timeval	 t2;
	struct timeval	 t;
	double		 speed;

	gettimeofday(&t1, NULL);
	func(ptr, size);
	gettimeofday(&t2, NULL);

	t.tv_sec = t2.tv_sec - t1.tv_sec;
	if (t1.tv_sec > t2.tv_sec) {
		t.tv_sec--;
		t.tv_usec = USEC_IN_SEC - t1.tv_usec + t2.tv_usec;
	} else {
		t.tv_usec = t2.tv_usec - t1.tv_usec;
	}

	speed = ((double)SIZE / ((double)t.tv_sec * USEC_IN_SEC + (double)t.tv_usec)) * USEC_IN_SEC;

	return speed;
}

int
main(int argc, char **argv)
{
	uint8_t		*ptr;
	size_t		 size = 1024 * 1024 * SIZE;
	double		 speed;

	ptr = malloc(size);
	if (ptr == NULL)
		return -1;

	printf("Read: %.1f MiB/s\n", measure(memread, ptr, size));
	printf("Write: %.1f MiB/s\n", measure(memwrite, ptr, size));

	return 0;
}
