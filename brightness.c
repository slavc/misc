/*
 * Adjust brightness of a laptop screen.
 */

#include <stdio.h>
#include <stdlib.h>

#define BRIGHTNESS_PATH		"/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-LVDS-1/intel_backlight/brightness"
#define MAX_BRIGHTNESS_PATH	"/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-LVDS-1/intel_backlight/max_brightness"
#define NUM_STEPS		20

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

int
main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return 0;
	}

	int	 cur;
	int	 max;
	int	 delta;

	cur = read_int(BRIGHTNESS_PATH);
	max = read_int(MAX_BRIGHTNESS_PATH);
	delta = max / NUM_STEPS;

	if (cur == -1 || max == -1)
		return 1;

	while (++argv, --argc) {
		if (**argv == '+')
			cur += delta;
		else if (**argv == '-')
			cur -= delta;
		else {
			usage();
			return 1;
		}
	}

	if (cur > max)
		cur = max;
	else if (cur < 0)
		cur = 0;

	if (write_int(BRIGHTNESS_PATH, cur) == -1)
		return 1;
	return 0;
}
