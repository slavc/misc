// by jeremie miller - 2010
// public domain, contributions/improvements welcome via github
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>


// opportunity to further optimize would be having different jump tables for higher depths
//#define PUSH(i) if(depth == 1) prev = *out++ = ((cur+i) - js)
//#define CAP(i) if(depth == 1) prev = *out++ = ((cur+i) - (js + prev) + 1)
#define PUSH(i) if(depth == 1) prev = ((cur+i) - js)
#define CAP(i) if(depth == 1) prev = ((cur+i) - (js + prev) + 1)

int
js0n(unsigned char *js, unsigned int len, unsigned short *out)
{
	unsigned short prev = 0;
	unsigned char *cur, *end;
	int depth=0;
	int utf8_remain=0;
	static void *gostruct[] = 
	{
		[0 ... 255] = &&l_bad,
		['\t'] = &&l_loop, [' '] = &&l_loop, ['\r'] = &&l_loop, ['\n'] = &&l_loop,
		['"'] = &&l_qup,
		[':'] = &&l_loop,[','] = &&l_loop,
		['['] = &&l_up, [']'] = &&l_down, // tracking [] and {} individually would allow fuller validation but is really messy
		['{'] = &&l_up, ['}'] = &&l_down,
		['-'] = &&l_bare, [48 ... 57] = &&l_bare, // 0-9
		['t'] = &&l_bare, ['f'] = &&l_bare, ['n'] = &&l_bare // true, false, null
	};
	static void *gobare[] = 
	{
		[0 ... 31] = &&l_bad,
		[32 ... 126] = &&l_loop, // could be more pedantic/validation-checking
		['\t'] = &&l_unbare, [' '] = &&l_unbare, ['\r'] = &&l_unbare, ['\n'] = &&l_unbare,
		[','] = &&l_unbare, [']'] = &&l_unbare, ['}'] = &&l_unbare,
		[127 ... 255] = &&l_bad
	};
	static void *gostring[] = 
	{
		[0 ... 31] = &&l_bad, [127] = &&l_bad,
		[32 ... 126] = &&l_loop,
		['\\'] = &&l_esc, ['"'] = &&l_qdown,
		[128 ... 191] = &&l_bad,
		[192 ... 223] = &&l_utf8_2,
		[224 ... 239] = &&l_utf8_3,
		[240 ... 247] = &&l_utf8_4,
		[248 ... 255] = &&l_bad
	};
	static void *goutf8_continue[] =
	{
		[0 ... 127] = &&l_bad,
		[128 ... 191] = &&l_utf_continue,
		[192 ... 255] = &&l_bad
	};
	static void *goesc[] = 
	{
		[0 ... 255] = &&l_bad,
		['"'] = &&l_unesc, ['\\'] = &&l_unesc, ['/'] = &&l_unesc, ['b'] = &&l_unesc,
		['f'] = &&l_unesc, ['n'] = &&l_unesc, ['r'] = &&l_unesc, ['t'] = &&l_unesc, ['u'] = &&l_unesc
	};
	static void **go = gostruct;
	
	for(cur=js,end=js+len; cur<end; cur++)
	{
			goto *go[*cur];
			l_loop:;
	}
	
	return depth; // 0 if successful full parse, >0 for incomplete data
	
	l_bad:
		return 1;
	
	l_up:
		PUSH(0);
		++depth;
		goto l_loop;

	l_down:
		--depth;
		CAP(0);
		goto l_loop;

	l_qup:
		PUSH(1);
		go=gostring;
		goto l_loop;

	l_qdown:
		CAP(-1);
		go=gostruct;
		goto l_loop;
		
	l_esc:
		go = goesc;
		goto l_loop;
		
	l_unesc:
		go = gostring;
		goto l_loop;

	l_bare:
		PUSH(0);
		go = gobare;
		goto l_loop;

	l_unbare:
		CAP(-1);
		go = gostruct;
		goto *go[*cur];

	l_utf8_2:
		go = goutf8_continue;
		utf8_remain = 1;
		goto l_loop;

	l_utf8_3:
		go = goutf8_continue;
		utf8_remain = 2;
		goto l_loop;

	l_utf8_4:
		go = goutf8_continue;
		utf8_remain = 3;
		goto l_loop;

	l_utf_continue:
		if (!--utf8_remain)
			go=gostring;
		goto l_loop;

}

int 
main(int argc, char **argv)
{
	char *buf = NULL;
	size_t len, bufsize;
	ssize_t nread;
	const size_t chunksize = 8192;
	int fd;
	const size_t usec_in_sec = 1000000;

	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		fd = open(*argv, O_RDONLY); 
		if (fd == -1) {
			perror(*argv);
			continue;
		}

		len = bufsize = nread = 0;
		buf = NULL;
		do {
			bufsize += chunksize;
			buf = realloc(buf, bufsize);
			len += nread;
			nread = read(fd, buf + len, chunksize);
		} while (nread == chunksize);
		close(fd);
		if (nread == -1) {
			perror("read");
			free(buf);
			continue;
		}
		len += nread;
		buf = realloc(buf, len + 1);
		buf[len] = '\0';

		struct timeval tv1, tv2;
		gettimeofday(&tv1, NULL);
		js0n(buf, len, NULL);
		gettimeofday(&tv2, NULL);

		unsigned long long usec1 = tv1.tv_sec * usec_in_sec + tv1.tv_usec;
		unsigned long long usec2 = tv2.tv_sec * usec_in_sec + tv2.tv_usec;
		unsigned long long dusec = usec2 - usec1;

		unsigned int sec = dusec / usec_in_sec;
		unsigned int usec = dusec % usec_in_sec;

		printf("%us %uus\n", sec, usec);

		free(buf);
	}

	return 0;
}
