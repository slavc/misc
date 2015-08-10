/*
 * Smoothly adjust brightness of a laptop screen.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BRIGHTNESS_PATH		"/sys/devices/pci0000:00/0000:00:02.0/backlight/acpi_video0/brightness"
#define MAX_BRIGHTNESS_PATH	"/sys/devices/pci0000:00/0000:00:02.0/backlight/acpi_video0/max_brightness"
#define NUM_STEPS		15
#define TRANS_TIME		(200 * 1000) // us

static void
usage(void)
{
	printf("usage: brightness + [+|-...]\n");
}

static int
read_int(const char *path)
{
	FILE	*fp;
	int	 i;

	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;
	if (fscanf(fp, "%d", &i) != 1)
		i = -1;
	fclose(fp);

	return i;
}

static int
write_int(const char *path, int i)
{
	FILE	*fp;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "%d\n", i);
	fclose(fp);

	return 0;
}

static int
transition(int cur, int desired, int duration)
{
	int	 delta;
	int	 step;
	int	 delay;

	delta = desired - cur;
	if (delta == 0)
		return 0;
	else if (delta < 0) {
		delta = -delta;
		step = -1;
	} else
		step = 1;

	delay = duration / delta;

	while (cur != desired) {
		cur += step;
		if (write_int(BRIGHTNESS_PATH, cur) == -1)
			return -1;
		usleep(delay);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return 0;
	}

	int	 cur;
	int	 max;
	int	 desired;
	int	 delta;

	cur = read_int(BRIGHTNESS_PATH);
	max = read_int(MAX_BRIGHTNESS_PATH);
	delta = max / NUM_STEPS;

	if (cur == -1 || max == -1)
		return 1;

	while (++argv, --argc) {
		if (**argv == '+')
			desired = cur + delta;
		else if (**argv == '-')
			desired = cur - delta;
		else {
			usage();
			return 1;
		}
	}

	if (desired > max)
		desired = max;
	else if (desired < 0)
		desired = 0;

	if (transition(cur, desired, TRANS_TIME) == -1)
		return 1;
	return 0;
}
