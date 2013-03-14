#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <err.h>

#include <ao/ao.h>

#define DEFAULT_WAVE_TYPE	SINE
#define DEFAULT_FREQUENCY	400

#define BITS		16
#define RATE		44100 
#define CHANNELS	1
#define BYTE_FORMAT	AO_FMT_NATIVE

int		 quit = 0;
static char	*wave_type_names[] = {
	"sine",
	"square",
	"sawtooth",
	"noise",
};
enum wave_types {
	SINE,
	SQUARE,
	SAWTOOTH,
	NOISE,
	INVALID,
};

static void	 usage(int);
static void	 tone(int, int);
static void	 handle_sig(int);

int
main(int argc, char **argv)
{
	int		 ch, wave_type, freq;
	extern char	*optarg;

	wave_type = DEFAULT_WAVE_TYPE;
	freq = DEFAULT_FREQUENCY;

	while ((ch = getopt(argc, argv, "hw:f:")) != -1) {
		switch (ch) {
		case 'w':
			for (wave_type = 0; wave_type != INVALID; ++wave_type)
				if (!strcmp(optarg, wave_type_names[wave_type]))
					break;
			if (wave_type == INVALID)
				usage(1);
			break;
		case 'f':
			freq = atoi(optarg);
			if (freq <= 0 || freq >= RATE/2)
				errx(1, "frequency must be between 0 and %d", RATE/2);
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	tone(wave_type, freq);

	return 0;
}

static void
usage(int exit_code)
{
	printf(
	    "usage: tone [options]\n"
	    "\n"
	    "Options:\n"
	    "	-h		output this message\n"
	    "	-w <waveform>	<waveform> is one of sine, square, sawtooth or noise\n",
	    "	-f <frequency>	<frequency> is an integer in range (0, %d)\n"
	    "\n"
	    "Produces a tone of given waveform and frequency, %s and %dHz by default.\n",
	    RATE/2,
	    wave_type_names[DEFAULT_WAVE_TYPE],
	    DEFAULT_FREQUENCY
	);
	exit(exit_code);
}

static void
tone(int wave_type, int freq)
{
	int			 id;
	ao_device		*dev;
	ao_sample_format	 sf;
	short			*buf;
	size_t			 bufsize;
	int			 i;

	/* Init libao. */
	ao_initialize();
	id = ao_default_driver_id();
	if (id == -1) {
		warnx("ao_default_driver_id failed");
		ao_shutdown();
		return;
	}
	memset(&sf, 0, sizeof sf);
	sf.bits = BITS;
	sf.rate = RATE;
	sf.channels = CHANNELS;
	sf.byte_format = BYTE_FORMAT;
	sf.matrix = NULL;
	dev = ao_open_live(id, &sf, NULL);
	if (dev == NULL) {
		warnx("ao_open_live failed");
		ao_shutdown();
		return;
	}

	/* Allocate a buffer for 1 second of sound. */
	bufsize = (sf.bits / CHAR_BIT) * sf.rate * sf.channels;
	buf = malloc(bufsize);
	if (buf == NULL) {
		warn("malloc");
		ao_shutdown();
		return;
	}

	/* Generate samples of given waveform and frequency. */
	switch (wave_type) {
	case SQUARE:
		for (i = 0; i < sf.rate; ++i)
			buf[i] = ((SHRT_MAX - 1) * sin((double) i * (((M_PI * 2.0) * (double) freq) / (double) sf.rate))) > 0 ? SHRT_MAX : SHRT_MIN;
		break;
	case SAWTOOTH:
		for (i = 0; i < sf.rate; ++i) {
			buf[i] = (SHRT_MAX * 2) * (double) i / (double) (sf.rate / freq) - SHRT_MAX;
		}
		break;
	case SINE:
	default:
		for (i = 0; i < sf.rate; ++i)
			buf[i] = (SHRT_MAX - 1) * sin((double) i * (((M_PI * 2.0) * (double) freq) / (double) sf.rate));
		break;
	}

	/* Set signal handlers. */
	signal(SIGHUP, handle_sig);
	signal(SIGINT, handle_sig);
	signal(SIGQUIT, handle_sig);
	signal(SIGTERM, handle_sig);

	/* Play the buffer. */
	while (!quit) {
		if (!ao_play(dev, (void *) buf, bufsize)) {
			warnx("ao_play failed");
			break;
		}
	}

	if (!ao_close(dev))
		warnx("ao_close failed");
	ao_shutdown();
}

static void
handle_sig(int unused)
{
	quit = 1;
}

