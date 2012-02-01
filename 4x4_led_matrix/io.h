#ifndef IO_H
#define IO_H

void enable_all_io_ports(void);
void enable_io_port(int port);
void writeRaw(unsigned short us);
void writeChar(char ch);
void normalizeText(char *s);
void scrollText(char *s, useconds_t delay);
void substText(char *s, useconds_t delay);
void setPixel(unsigned short x, unsigned short y);
void clrPixel(unsigned short x, unsigned short y);

#endif

