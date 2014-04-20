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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <err.h>

#include <Python.h>

#include "portio.h"

static int iopl_rc;

static PyObject *
portio_enable_io(PyObject * self, PyObject * args)
{
	if ((iopl_rc = SET_IOPL(SET_IOPL_ARG)) == SET_IOPL_ERROR)
		Py_RETURN_FALSE;
	else
		Py_RETURN_TRUE;
}


static PyObject *
portio_outb(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "ii", &port, &data))
		return NULL;
	OUTB(port, data);
	Py_RETURN_NONE;
}

static PyObject *
portio_outw(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "ii", &port, &data))
		return NULL;
	OUTW(port, data);
	Py_RETURN_NONE;
}

static PyObject *
portio_outl(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "ii", &port, &data))
		return NULL;
	OUTL(port, data);
	Py_RETURN_NONE;
}

static PyObject *
portio_inb(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "i", &port))
		return NULL;
	data = INB(port);
	return PyInt_FromLong(data);
}

static PyObject *
portio_inw(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "i", &port))
		return NULL;
	data = INW(port);
	return PyInt_FromLong(data);
}

static PyObject *
portio_inl(PyObject * self, PyObject * args)
{
	int             port;
	unsigned long   data;

	if (!PyArg_ParseTuple(args, "i", &port))
		return NULL;
	data = INL(port);
	return PyInt_FromLong(data);
}

static PyMethodDef portio_methods[] = {
	{"enable_io", portio_enable_io, METH_VARARGS, "enable_io()\nEnables access to I/O ports."},
	{"outb", portio_outb, METH_VARARGS, "outb(port, byte)\nWrite a byte into port."},
	{"outw", portio_outw, METH_VARARGS, "outw(port, word)\nWrite a word into port."},
	{"outl", portio_outl, METH_VARARGS, "outl(port, long)\nWrite a long into port."},
	{"inb", portio_inb, METH_VARARGS, "inb(port)\nRead a byte from port."},
	{"inw", portio_inw, METH_VARARGS, "inw(port)\nRead a word from port."},
	{"inl", portio_inl, METH_VARARGS, "inl(port)\nRead a long from port."},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initportio(void)
{
	Py_InitModule("portio", portio_methods);
}
