/* Reads a file with notes and plays them back on the PC-speaker. */

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
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <err.h>
#ifdef OPENBSD
#	include <i386/pio.h>
#else
#	include <sys/io.h>
#endif

#define MIN_OCTAVE 0
#define MAX_OCTAVE 8
#define IS_VALID_OCTAVE(x) (x >= MIN_OCTAVE && x <= MAX_OCTAVE)
#define IS_VALID_NOTE(x) (((x) >= 'A' && (x) <= 'G') || ((x) >= 'a' && (x) <= 'g'))
#define NOTE_TO_INDEX(x) (note_to_index[tolower(x)-'a'])

struct note {
	int             freq;
	int             period;
};

const int       note_to_index[] = {5, 6, 0, 1, 2, 3, 4};
const int       freqs[MAX_OCTAVE - MIN_OCTAVE + 1][7] = {
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
int             octave;
int             baseoctave;
int             dflag;

void
init_timer(int freq)
{
	unsigned char   cb = 0x80 | 0x06 | 0x30;	/* timer 2; leas/most;
							 * mode 2, binary */

	outb(0x43, cb);
	outb(0x42, ((1193182 + (freq / 2)) / freq) % 256);
	outb(0x42, ((1193182 + (freq / 2)) / freq) / 256);
}

void
play_sound(int freq, int period)
{
	unsigned char   b;

	init_timer(freq);

	b = inb(0x61);
	outb(0x61, b | 0x2 | 0x1);
	usleep(period);
	outb(0x61, b & ~(0x2 | 0x1));
}

int
starts_with(const char *bigstr, const char *smallstr)
{
	while (*smallstr == *bigstr && *smallstr != '\0') {
		++bigstr;
		++smallstr;
	}

	return !*smallstr;
}

char           *
skip_space(const char *s)
{
	while (isspace(*s))
		++s;

	return (char *) s;
}

void
syntax_error(int linenum, const char *fmt,...)
{
	extern char    *__progname;
	va_list         ap;

	fprintf(stderr, "%s: syntax error on line %i", __progname, linenum);
	if (fmt == NULL) {
		fputc('\n', stderr);
		return;
	}
	fprintf(stderr, ": ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

struct note
read_note(const char *s, int linenum)
{
	static struct note note;
	static int      last_period = 500;
	char            c;
	char            notestr[3] = "\0\0";
	int             oct = octave;
	int             period;
	int             n;

	n = sscanf(s, "%2s%i", notestr, &period);
	if (n < 2)
		period = last_period;
	else
		last_period = period;
	if (n < 1) {
		syntax_error(linenum, NULL);
		exit(1);
	}
	c = notestr[0];
	if (!IS_VALID_NOTE(c) && c != 'p') {
		syntax_error(linenum, "invalid note");
		exit(1);
	}
	if (notestr[1] != '\0') {
		oct = atoi(notestr + 1);
		if (!IS_VALID_OCTAVE(oct)) {
			syntax_error(linenum, "octave must be between %i and %i", MIN_OCTAVE, MAX_OCTAVE);
			exit(1);
		}
	} else {
		oct = octave;
		if (!IS_VALID_OCTAVE(baseoctave + oct)) {
			syntax_error(linenum, "(baseoctave = %i; octave = %i;) octave must be between %i and %i",
				   baseoctave, oct, MIN_OCTAVE, MAX_OCTAVE);
			exit(1);
		}
	}

	if (c == 'p')
		note.freq = -1;
	else
		note.freq = freqs[oct][NOTE_TO_INDEX(c)];
	note.period = period;

	return note;
}

void
play_note(struct note note)
{
    if (dflag)
	    printf("PLAY_NOTE: freq = %i; period = %i;\n", note.freq, note.period);
	if (note.freq == -1)
		usleep(note.period * 1000);
	else
		play_sound(note.freq, note.period * 1000);
}

void
play_file(FILE * infp, FILE * outfp)
{
	struct note     note;
	int             linenum = 0, n;
	const size_t    bufsiz = 1024;
	char            buf[bufsiz];
	char           *s, *p;

	while (fgets(buf, bufsiz, infp) != NULL) {
		++linenum;
		s = skip_space(buf);
		if (*s == '\0') {
            if (dflag)
			    printf("BLANK LINE\n");
			continue;
		}
		if (*s == '#') {
            if (dflag)
			    printf("COMMENT\n");
			continue;
		}
		if (starts_with(buf, "octave")) {
			if ((p = strchr(s, ' ')) == NULL && (p = strchr(s, '\t')) == NULL) {
				syntax_error(linenum, "'octave' directive with no operand");
				exit(1);
			} else {
				octave = atoi(p);
				if (!IS_VALID_OCTAVE(octave)) {
					syntax_error(linenum, "octave must be between %i and %i", MIN_OCTAVE, MAX_OCTAVE);
					exit(1);
				}
                if (dflag)
				    printf("OCTAVE: %i\n", octave);
			}
			continue;
		}
		if (starts_with(buf, "baseoctave")) {
			if ((p = strchr(s, ' ')) == NULL && (p = strchr(s, '\t')) == NULL) {
				syntax_error(linenum, "'baseoctave' directive with no operand");
				exit(1);
			} else {
				baseoctave = atoi(p);
                if (dflag)
				    printf("BASEOCTAVE: %i\n", baseoctave);
			}
			continue;
		}
		note = read_note(s, linenum);
        if (outfp)
            fwrite(&note, sizeof(struct note), 1, outfp);
        else
		    play_note(note);
	}
}

void
pcspkr_play(const char *infilename, const char *outfilename)
{
	FILE           *infp, *outfp;

    infp = outfp = NULL;
	if ((infp = fopen(infilename, "r")) == NULL)
		err(2, "\"%s\"", infilename);
    if (outfilename) {
        if ((outfp = fopen(outfilename, "wb")) == NULL)
            err(2, "\"%s\"", outfilename);
    }   

	play_file(infp, outfp);

	fclose(infp);
    if (outfp)
        fclose(outfp);
}

void
usage(void)
{
	extern char    *__progname;

	printf("usage: %s [-d] input_file [output_file]\n"
        "If output_file given -- write a binary file with frequencies and periods (both get written as integers)\n"
        "\t-d\t-- output debug info\n",
        __progname);

	exit(0);
}

void
set_iopl(void)
{
#ifdef OPENBSD
	if (i386_iopl(3) == -1) {
#else
	if (iopl(3) == -1) {
#endif
		perror(NULL);
		exit(1);
	}
}

int
main(int argc, char **argv)
{
    int i;
    char *infilename, *outfilename;
    
    infilename = outfilename = NULL;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            usage();
        else if (!strcmp(argv[i], "-d"))
            ++dflag;
        else if (infilename == NULL)
            infilename = argv[i];
        else if (outfilename == NULL)
            outfilename = argv[i];
        else
            usage();
    }
    if (!infilename)
        usage();

    if (!outfilename)
        set_iopl();

	pcspkr_play(infilename, outfilename);

	exit(0);
}
