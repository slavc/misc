#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <i386/pio.h>
#include <machine/sysarch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "char_bitmaps.h"



#define DEBUG


extern int      errno;

unsigned short current_bitmap;

/*
 * Enable access to ALL I/O ports, by modifying the I/O Privilege Level
 * (IOPL) field in the EFLAGS register.
 */
void
enable_all_io_ports(void)
{
        if (i386_iopl(3) != 0)
                err(errno, "i386_iopl");
}

/*
 * Modifies the I/O permission bitmap for the current process, so that the
 * process could read/write the I/O port 'port'.
 */
void
enable_io_port(int port)
{
        u_long          iomap[32];

        if (i386_get_ioperm(iomap) != 0)
                err(errno, "i386_get_permio");

        iomap[((unsigned short) port) / (4 * 8)] &= ~(((u_long) 1) << (((unsigned short) port) % (4 * 8)));

        if (i386_set_ioperm(iomap) != 0)
                err(errno, "i386_set_permio");
}

void writeRaw(unsigned short us){
	outb( 0x37a, 0 );
	outb( 0x378, (unsigned char) us );

	outb( 0x37a, 4 );
	outb( 0x378, (unsigned char) (us >> 8) );

	current_bitmap = us;
}

void writeChar(char ch){
	extern unsigned short char_bitmap_tbl[];

	ch = toupper( ch );

	if( (! (ch >= 'A' && ch <= 'Z')) || ch == ' ' ){
		writeRaw( 0 );
		return;
	}

	writeRaw( char_bitmap_tbl[ch - 'A'] );
}

void normalizeText(char *s){

	while( *s != '\0' ){
		*s = toupper( *s );
		if( ! (*s >= 'A' && *s <= 'Z') )
			*s = ' ';
		++s;
	}
}

void scrollText(char *s, useconds_t delay){
	char *p;
	unsigned short us=0;
	int i;
	unsigned short curchar, nexchar, l0c, l1c, l2c, l3c, l0n, l1n, l2n, l3n, l0, l1, l2, l3; /* char bitmaps */
	extern unsigned short char_bitmap_tbl[];

	normalizeText( s );

	while( *s != '\0' ){
		if( *s == ' ' ){
			curchar = 0;
			l0c = l1c = l2c = l3c = 0;
		} else {
			curchar = char_bitmap_tbl[*s-'A'];

			l0c = GET_LINE_0(curchar);
			l1c = GET_LINE_1(curchar);
			l2c = GET_LINE_2(curchar);
			l3c = GET_LINE_3(curchar);
		}

		if( *(s+1) == '\0' || *(s+1) == ' ' ){
			nexchar = 0;
			l0n = l1n = l2n = l3n = 0;
		} else {
			nexchar = char_bitmap_tbl[*(s+1)-'A'];

			l0n = GET_LINE_0(nexchar);
			l1n = GET_LINE_1(nexchar);
			l2n = GET_LINE_2(nexchar);
			l3n = GET_LINE_3(nexchar);
		}


		for( i=0; i < 4; ++i ){
			l0 = (l0c << i) | (l0n >> (4-i));
			l1 = (l1c << i) | (l1n >> (4-i));
			l2 = (l2c << i) | (l2n >> (4-i));
			l3 = (l3c << i) | (l3n >> (4-i));

			us = LINE_0(l0) | LINE_1(l1) | LINE_2(l2) | LINE_3(l3);

			/*
			us = LINE_0( (GET_LINE_0(curchar) << i) | (GET_LINE_0(nexchar) >> (4-i)) ) | \
				LINE_1( (GET_LINE_1(curchar) << i) | (GET_LINE_1(nexchar) >> (4-i)) ) | \
				LINE_2( (GET_LINE_2(curchar) << i) | (GET_LINE_2(nexchar) >> (4-i)) ) | \
				LINE_3( (GET_LINE_3(curchar) << i) | (GET_LINE_3(nexchar) >> (4-i)) );
			*/

			writeRaw( us );
			usleep( delay );
		}

		++s;
	}

	writeRaw( 0 );
}

void substText(char *s, useconds_t delay){
	normalizeText( s );

	while( *s != '\0' ){
		writeChar( *s );
		usleep( delay );
		++s;
	}
	writeRaw( 0 );
}

void setPixel(unsigned short x, unsigned short y){
	unsigned short us;

	x %= 4;
	y %= 4;

	us = current_bitmap | (1 << x) << (4 * y);

	writeRaw( us );
}

void clrPixel(unsigned short x, unsigned short y){
	unsigned short us;

    x %= 4;
    y %= 4;

    us = current_bitmap & ~((1 << x) << (4 * y));

    writeRaw( us );
}



void dispDemo(useconds_t delay){
	int cnt, i, j;

	/* circle */
	for( cnt=0; cnt < 4; ++cnt ){
		for( i=0, j=0; i < 4; ++i ){
			if( cnt != 3 )
				writeRaw( 0 );
			setPixel( i, j );
			usleep( delay );
		}
		for( i=3, j=0; j < 4; ++j ){
			if( cnt != 3 )
				writeRaw( 0 );
			setPixel( i, j );
			usleep( delay );
		}
		for( i=3, j=3; i > -1; --i ){
			if( cnt != 3 )
				writeRaw( 0 );
			setPixel( i, j );
			usleep( delay );
		}
		for( i=0, j=3; j > -1; --j ){
			if( cnt != 3 )
				writeRaw( 0 );
			setPixel( i, j );
			usleep( delay );
		}
	}
	
	writeRaw( 0 );
}

