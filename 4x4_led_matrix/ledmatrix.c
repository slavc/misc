#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "char_bitmaps.h"
#include "io.h"



#define DEBUG

#define OPTSTRING "t:sbdrD"

#define DEFAULT_DELAY 300000



extern int errno;



char *argv_0;

enum e_modes{
	MODE_SCROLL,
	MODE_SUBST,
	MODE_DISPCHAR,
	MODE_RAW,
	MODE_DEMO
};

struct{
	unsigned int mode:3;
} flags;




void usage(void){
	printf( "usage: %s [-t <delay>] [-s || -b || -d || -r || -D] <data>\n\n"\
		"\t-t <delay> -- delay for scrolling or substituting display modes, see below\n"
		"\t-s -- default mode, scroll text; 'data' is the text to scroll\n"\
		"\t-b -- display text by substituting chars; 'data' is the text to display\n"\
		"\t-d -- display an individual char; 'data' is the char\n"\
		"\t-r -- raw; 'data' is an unsigned short value to write to the matrix\n"\
		"\t-D -- display a demo; 'data' is ignored\n\n"\
		"examples:\n\n"\
		"\t%s \"WHAT HAPPEN !!\" -- scroll text \"WHAT HAPPEN !!\" with the default speed\n"\
		"\t%s -t 100000 \"GO ZIG !!\" -- will scroll the text \"GO ZIG\", delay for moving pixels is 100000 microseconds\n"
		"\t%s -t 100000 \"FSCK\" -- will display the text \"FSCK\" by substituting the characters with the speed of 100000 microseconds\n"
		"\t%s -d A -- display character 'A'\n"\
		"\t%s -r 15 -- lighten up the first line of the matrix\n",
		argv_0, argv_0, argv_0, argv_0, argv_0, argv_0, argv_0 );
}

int main(int argc, char **argv){
	int ch;
	char *text;
	useconds_t delay;
	unsigned int ui;
	extern char *optarg;
	extern int optind;

	argv_0 = argv[0];

	if( argc < 2 ){
		usage( );
		return 0;
	}

	text = NULL;
	delay = DEFAULT_DELAY;
	flags.mode = MODE_SCROLL;

	while( (ch = getopt( argc, argv, OPTSTRING )) != -1 ){
		switch( ch ){
			case 't':
				sscanf( optarg, "%u", &delay );
				break;

			case 's':
				flags.mode = MODE_SCROLL;
				break;

			case 'b':
				flags.mode = MODE_SUBST;
				break;

			case 'd':
				flags.mode = MODE_DISPCHAR;
				break;

			case 'r':
				flags.mode = MODE_RAW;
				break;

			case 'D':
				flags.mode = MODE_DEMO;
				break;

			default:
				usage( );
				return 0;
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if( argc != 0 )
		text = argv[0];
	else if( flags.mode != MODE_DEMO ){
		usage( );
		return 1;
	}

	enable_io_port( 0x378 );
	enable_io_port( 0x379 );
	enable_io_port( 0x37a );

	switch( flags.mode ){
		default:
		case MODE_SCROLL:
			scrollText( text, delay );
			break;

		case MODE_SUBST:
			substText( text, delay );
			break;

		case MODE_DISPCHAR:
			writeChar( *text );
			break;

		case MODE_RAW:
			ui = 0;
			sscanf( text, "%u", &ui );
			writeRaw( (unsigned short) ui );
			break;

		case MODE_DEMO:
			dispDemo( delay );
			break;
	}

	return 0;
}

