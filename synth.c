/* Reads a file with notes and synthesizes them.
 * (OpenBSD only.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sndio.h>

#define PI   3.14159265358979323846264338327950288
#define PI2 (PI * 2)

extern const char *__progname;

const int       note_to_index[] = {5, 6, 0, 1, 2, 3, 4};
const int       freqs[9][7] = {
	{16, 18, 21, 22, 25, 28, 31},	/* 0 */
	{33, 37, 41, 44, 49, 55, 62},	/* 1 */
	{65, 73, 82, 87, 98, 110, 123},	/* 2 */
	{131, 147, 165, 175, 196, 220, 247},	/* 3 */
	{262, 294, 330, 349, 392, 440, 494},	/* 4 */
	{523, 587, 659, 698, 784, 880, 988},	/* 5 */
	{1047, 1175, 1319, 1397, 1568, 1760, 1976},	/* 6 */
	{2093, 2349, 2637, 2794, 3136, 3520, 3951},	/* 7 */
	{4186, 4698, 0, 0, 0, 0, 0}	/* 8 */
};
/* C     D     E     F     G     A     B       */

enum e_waveforms {
    SINE,
    SQUARE,
    TRIANGLE,
    SAWTOOTH,
    NOISE
};

struct note {
    char note;
    int octave;
    int len; // how long to play msecs 
    int waveform;
    struct note *next;
};

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
typedef double (*wavegen_func_t)(double);
/*double (*wavegen_func)(double);
wavegen_func_t wavegen_func;*/
double
note_to_freq(int octave, char note) {
    if (tolower(note) == 'p')
        return 0.0;
    assert(octave >= 0 && octave <= 8);
    assert(strchr("abcdefg", tolower(note)));
    return (double) freqs[octave][note_to_index[tolower(note) - 'a']];
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
audio_write(struct audio *au, short buf) {
    int i;

    for (i = 0; i < au->params.pchan; ++i)
        if (!sio_write(au->dev, &buf, sizeof(buf)))
            err(1, "sio_write");
}

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

double
silence(double unused) {
    return 0.0;
}

wavegen_func_t wavegen_funcs[] = {
    sin,
    square,
    triangle,
    sawtooth,
    noise
};


char *
skip_space(const char *s) {
    while (isspace(*s))
        ++s;
    return (char *) s;
}

char *
token(char **strp) {
    char *s, *start;

    if (*strp == NULL)
        return NULL;

    start = s = skip_space(*strp);
    while (!isspace(*s) && *s != '\0')
        ++s;
    if (s == start) {
        *strp = NULL;
        return NULL;
    }

    if (*s != '\0') {
        *s = '\0';
        *strp = s + 1;
    } else
        *strp = NULL;

    return start;
}

int
tokenize(char *s, char ***pwords) {
    /* char **words; */
    char *tok;
    int n = 0;
    static char **words = NULL;
    
    while (tok = token(&s)) {
        words = realloc(words, sizeof(char *) * ++n);
        words[n-1] = tok;
    }

    if (pwords)
        *pwords = words;
    else {
        free(words);
        words = NULL;
    }

    return n;
}

void
free_notes(struct note *notes, int n_notes) {
    int i;
    struct note *next, *p;

    for (i = 0; i < n_notes; ++i) {
        next = notes[i].next;
        while (p = next) {
            next = p->next;
            free(p);
        }
    }

    free(notes);
}

int
syntax_error(int line_no, const char *fmt, ...) {
    va_list ap;

    fprintf(stderr, "%s: syntax error on line %i", __progname, line_no);

    if (fmt) {
        fprintf(stderr, ": ");
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }

    fprintf(stderr, "\n");

    return -1;
}

int
parse_file(FILE *fp, struct note **pnotes, int *pn_notes) {
    struct note *notes = NULL;
    int n_notes = 0;
    struct note note;

    char line[1024];
    const size_t line_size = sizeof(line);
    int line_no = 0;

    char **words;
    int n_words;
    int wordi; // index for words array

    int octave = 3;
    int len = 200;
    int waveform = SINE;

    while (fgets(line, line_size, fp)) {
        ++line_no;

        n_words = tokenize(line, &words);

        if (n_words == 0 || words[0][0] == '#')
            continue;

        if (!strcmp(words[0], "octave")) {
            if (!words[1])
                syntax_error(line_no, "'octave' must have a numeric argument");
            octave = atoi(words[1]);
            continue;
        } else if (!strcmp(words[0], "sawtooth")) {
            waveform = SAWTOOTH;
            continue;
        } else if (!strcmp(words[0], "sine")) {
            waveform = SINE;
            continue;
        } else if (!strcmp(words[0], "triangle")) {
            waveform = TRIANGLE;
            continue;
        } else if (!strcmp(words[0], "square")) {
            waveform = SQUARE;
            continue;
        } else if (!strcmp(words[0], "noise")) {
            waveform = NOISE;
            continue;
        } else if (isdigit(words[0][0])) {
            len = atoi(words[0]);
            continue;
        } else if (!isalpha(words[0][0])) {
            return syntax_error(line_no, "unrecognized word ``%s''", words[0]);
        }

        memset(&note, 0, sizeof(note));
        note.waveform = waveform;
        note.octave = octave;
        note.len = len;

        for (wordi = 0; wordi < n_words; ++wordi) {
            if (strchr("abcdefgp", tolower(words[wordi][0]))) {
                note.note = tolower(words[wordi][0]);
                note.octave = octave;
                if (isdigit(words[wordi][1])) {
                    note.octave = atoi(words[wordi] + 1);
                }
            } else if (!strcmp(words[wordi], "sawtooth")) {
                note.waveform = SAWTOOTH;
            } else if (!strcmp(words[wordi], "sine")) {
                note.waveform = SINE;
            } else if (!strcmp(words[wordi], "triangle")) {
                note.waveform = TRIANGLE;
            } else if (!strcmp(words[wordi], "square")) {
                note.waveform = SQUARE;
            } else if (!strcmp(words[wordi], "noise")) {
                note.waveform = NOISE;
            } else if (isdigit(words[wordi][0])) {
                note.len = atoi(words[wordi]);
            } else {
                return syntax_error(line_no, "unrecognized word ``%s''", words[wordi]);
            }
        }

        ++n_notes;
        notes = realloc(notes, sizeof(note) * n_notes);
        notes[n_notes - 1] = note;
    }

    if (pnotes)
        *pnotes = notes;
    else
        free_notes(notes, n_notes);

    if (pn_notes)
        *pn_notes = n_notes;

    return 0;
}

void
print_notes(struct note *notes, int n_notes) {
    int i;

    const char *waveforms[] = {
        "SINE",
        "SQUARE",
        "TRIANGLE",
        "SAWTOOTH",
        "NOISE"
    };

    for (i = 0; i < n_notes; ++i) {
        printf("%c%i %s %i\n", notes[i].note, notes[i].octave, waveforms[notes[i].waveform], notes[i].len);
    }
}

void
play_notes(struct note *notes, int n_notes) {
    int notei, n_samples, i, j;
    struct audio au;
    double t, tstep, fval;
    double freq;
    short buf;
    short maxval = 0x7fff;
    wavegen_func_t wavegen_func;

    memset(&au, 0, sizeof(au));
    open_audio_device(&au);

    for (notei = 0; notei < n_notes && !g_interrupt_flag; ++notei) {
        freq = note_to_freq(notes[notei].octave, notes[notei].note);
        n_samples = ((double) notes[notei].len / 1000.0) * (double) au.params.rate;
        t = 0.0;
        tstep = freq * PI2 / (double) au.params.rate;
        if (notes[notei].note == 'p')
            wavegen_func = silence;
        else
            wavegen_func = wavegen_funcs[notes[notei].waveform];

        for (i = 0; i < n_samples && !g_interrupt_flag; ++i) {
            fval = wavegen_func(t);
            t += tstep;
            t = fmod(t, PI2);
            buf = fval * maxval;
            for (j = 0; j < au.params.pchan; ++j)
                audio_write(&au, buf);
        }
    }

    close_audio_device(&au);
}

int
synth(const char *filename) {
    FILE *fp;
    struct note *notes = NULL;
    int n_notes = 0;

    if ((fp = fopen(filename, "r")) == NULL) {
        warn("%s", filename);
        return -1;
    }
    if (parse_file(fp, &notes, &n_notes) == -1)
        return -1;
    print_notes(notes, n_notes);
    play_notes(notes, n_notes);
    free_notes(notes, n_notes);

    return 0;
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


/*
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
*/


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


void
usage(void) {
    const char *c, **p;

    printf("%s [-s|-t|-w] <frequency>|-n\n"
           "    Produce a <frequency> Hz tone. By default, sine wave is used.\n",
           __progname);
    for (c = OPTSTRING, p = optdescrs; *c && *p; ++c) {
        if (*c == ':')
            continue;
        printf("    -%c -- %s\n", *c, *p++);
    }
}

/*
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
*/

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

void
test_token(char *s) {
    char **words;
    int i, n;

    n = tokenize(s, &words);
    for (i = 0; i < n; ++i)
        printf("%i: \"%s\"\n", i, words[i]);
}

int
main(int argc, char **argv) {
    int i;

    srandomdev();

    signal(SIGINT, sig_handler);

    for (i = 1; i < argc; ++i)
        synth(argv[i]);

/*
    struct audio audio;


    memset(&audio, 0, sizeof(audio));

    srandomdev();

    proc_args(argc, argv);
    signal(SIGINT, sig_handler);

    open_audio_device(&audio);
    tonegen(&audio, g_frequency);
    close_audio_device(&audio);

    exit(EXIT_SUCCESS);
*/
}
