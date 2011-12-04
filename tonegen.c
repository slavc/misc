/* Generate a tone of the given frequency and wave-form.
 * (OpenBSD only).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sndio.h>

#define PI  M_PI
#define PI2 (PI * 2)

/* default audio parameters */
#define AUP_BITS  16
#define AUP_BPS   SIO_BPS(AUP_BITS)
#define AUP_SIG   1
#define AUP_LE    SIO_LE_NATIVE
#define AUP_PCHAN 2
#define AUP_RATE  44100

struct audio {
    struct sio_hdl *dev;
    struct sio_par params;
};

int    g_interrupt_flag;
double g_frequency;
/*
 * The function which given a radian value generates
 * a new value. For example sin() from math.
 */
double (*wavegen_func)(double);

double sign(double a) {
    if (a < 0.0)
        return -1.0;
    else if (a == 0.0)
        return 0.0;
    else
        return 1.0;
}

double
square(double t) {
    return sign(sin(t));
    /*
    t = fmod(t, PI);
    if (t < PI/2)
        return 1.0;
    else
        return -1.0;
    */
}

double
triangle(double t) {
    t = fmod(t, PI2);

    if (t <= PI/2) {
        return t * (1.0 / (PI/2));
    } else if (t <= (PI2 - PI/2)) {
        return (((t - (PI/2)) * (2.0 / PI)) - 1.0) * (-1.0);
    } else {
        return (t - (PI2 - PI/2)) * (1.0 / (PI/2)) - 1.0;
    }
}

double
sawtooth(double t) {
    t = fmod(t, PI2);
    return t * (2.0/PI2) - 1.0;
}

double
noise(double t) {
    static long val = -1;
    static double tval = 0.0;
    static double prevt = 0.0;

    if (t < prevt) {
        tval += t + PI2 - prevt;
    } else {
        tval += t - prevt;
    }
    prevt = t;

    if (tval >= PI2 || val == -1) {
        val = random();
        tval = fmod(val, PI2);
    }

    return (double) val * (2.0  / (double) 0x7fffffff) - 1.0;
}

void *
xalloc(void *p, size_t nbytes) {
    if ((p = realloc(p, nbytes)) == NULL)
        err(1, "xalloc(%lu)", nbytes);
    return p;
}

void
sig_handler(int signum) {
    if (signum == SIGINT)
        g_interrupt_flag = 1;
}

void
audio_write(struct audio *au, short buf) {
    int i;

    for (i = 0; i < au->params.pchan; ++i)
        if (!sio_write(au->dev, &buf, sizeof(buf)))
            err(1, "sio_write");
}

void
tonegen(struct audio *au, double freq) {
    double t, tstep;
    double fval;
    short buf;
    const short maxval = 0x7fff;
    int i;

    t = 0.0;
    tstep = (freq * PI2) / (double) au->params.rate;

    for ( ;; ) {
        fval = wavegen_func(t);
        t += tstep;
        t = fmod(t, PI2);
        buf = fval * maxval;
        for (i = 0; i < au->params.pchan; ++i)
            audio_write(au, buf);
        if (g_interrupt_flag)
            return;
    }
}

void
tonegen_debug(struct audio *au, double freq) {
    
}

int
is_tty(int fd) {
    int tgrp, rc;

    rc = ioctl(fd, TIOCGPGRP, &tgrp);

    return !(rc == -1 && errno == ENOTTY);
}

#define OPTSTRING "stwn"
const char *optdescrs[] = {
    "square wave",
    "triangle wave",
    "sawtooth wave",
    "noise",
    NULL
};

extern const char *__progname;

void
usage(void) {
    const char *c, **p;

    printf("%s [-s|-t|-w|-n] <freq>\n"
           "    Produce a <freq> Hz tone. By default, sine wave is used.\n",
           __progname);
    for (c = OPTSTRING, p = optdescrs; *c && *p; ++c) {
        if (*c == ':')
            continue;
        printf("    -%c -- %s\n", *c, *p++);
    }
}

extern char *optarg;
extern int optind;

void
proc_args(int argc, char **argv) {
    int ch;

    while ((ch = getopt(argc, argv, OPTSTRING)) != -1) {
        switch (ch) {
        case 's':
            wavegen_func = square;
            break;
        case 't':
            wavegen_func = triangle;
            break;
        case 'w':
            wavegen_func = sawtooth;
            break;
        case 'n':
            wavegen_func = noise;
            break;
        default:
            usage();
            exit(0);
            break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc)
        g_frequency = atoi(*argv);

    if (!wavegen_func)
        wavegen_func = sin;

    if (wavegen_func != noise && (g_frequency == 0.0 || g_frequency < 0.0 || g_frequency > 48000.0)) {
        usage();
        exit(0);
    }
}

void
open_audio_device(struct audio *au) {
    if ((au->dev = sio_open(NULL, SIO_PLAY, 0)) == NULL)
        err(1, "sio_open");

    sio_initpar(&au->params);
    au->params.bits  = AUP_BITS;
    au->params.bps   = AUP_BPS;
    au->params.sig   = AUP_SIG;
    au->params.le    = AUP_LE;
    au->params.pchan = AUP_PCHAN; 
    au->params.rate  = AUP_RATE;

    if (!sio_setpar(au->dev, &au->params))
        err(1, "sio_setpar");
    if (!sio_getpar(au->dev, &au->params))
        err(1, "sio_getpar");

#define ARE_AU_PARAMS_OKAY()   (au->params.bits  == AUP_BITS  && \
                                au->params.bps   == AUP_BPS   && \
                                au->params.sig   == AUP_SIG   && \
                                au->params.le    == AUP_LE    && \
                                au->params.pchan == AUP_PCHAN && \
                                au->params.rate  == AUP_RATE     )
    if (!ARE_AU_PARAMS_OKAY())
        errx(1, "failed to set desired audio parameters");

    if (!sio_start(au->dev))
        err(1, "sio_start");
}

void
close_audio_device(struct audio *au) {
    if (!sio_stop(au->dev))
        err(1, "sio_stop");
    sio_close(au->dev);
}

void
print_audio_device_info(struct audio *au) {
    const char *fmt = "Audio device info\n"
                      "bits:  %5u\n"
                      "bps:   %5u\n"
                      "sig:   %5u\n"
                      "le:    %5u\n"
                      "pchan: %5u\n"
                      "rate:  %5u\n";
    printf(fmt,
           au->params.bits,
           au->params.bps,
           au->params.sig,
           au->params.le,
           au->params.pchan,
           au->params.rate);
}

int
main(int argc, char **argv) {
    struct audio audio;

    memset(&audio, 0, sizeof(audio));

    srandomdev();

    proc_args(argc, argv);
    signal(SIGINT, sig_handler);

    open_audio_device(&audio);
    /* print_audio_device_info(&audio); */
    tonegen(&audio, g_frequency);
    close_audio_device(&audio);

    exit(EXIT_SUCCESS);
}
