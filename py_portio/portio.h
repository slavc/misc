/*
 * Copyright (c) 2009 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#ifndef PORTIO_H
#define PORTIO_H

#if defined(OS_LINUX)
#	include <sys/io.h>
#	include <sys/perm.h>
#	define SET_IOPL iopl
#	define SET_IOPL_ARG 3
#	define SET_IOPL_ERROR -1
#	define OUTB(port,data) outb(data,port)
#	define OUTW(port,data) outw(data,port)
#	define OUTL(port,data) outl(data,port)
#	define INB inb
#	define INW inw
#	define INL inl
#elif defined(OS_OPENBSD)
#	if defined(ARCH_AMD64)
#		include <amd64/pio.h>
#		define SET_IOPL amd64_iopl
#	elif defined(ARCH_I386)
#		include <i386/pio.h>
#		define SET_IOPL i386_iopl
#	endif
#	define SET_IOPL_ARG 3
#	define SET_IOPL_ERROR -1
#	define OUTB outb
#	define OUTW outw
#	define OUTL outl
#	define INB inb
#	define INW inw
#	define INL inl
#elif defined(OS_FREEBSD)
#	include <machine/cpufunc.h>
#	if !defined(ARCH_AMD64) && !defined(ARCH_I386)
#		error "Only AMD64 and i386 architectures are supported."
#	endif
#	define SET_IOPL open
#	define SET_IOPL_ARG "/dev/io"
#	define SET_IOPL_ERROR -1
#	define OUTB outb
#	define OUTW outw
#	define OUTL outl
#	define INB inb
#	define INW inw
#	define INL inl
#elif defined(OS_NETBSD)
#	error "NetBSD support not implemented yet."
#	include <x86/pio.h>
#else
#	error "This OS is not supported."
#endif

#endif
