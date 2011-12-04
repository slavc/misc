#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <err.h>
#include <sys/time.h> 

int
load_text_file(const char *filename, char **pbuf, size_t *pbufsize) {
    FILE *fp;
    int rc;

    if ((fp = fopen(filename, "r")) == NULL)
        return -1; 
    rc = f_load_text_file(fp, pbuf, pbufsize);
    fclose(fp); 

    return rc;
}

int
f_load_text_file(FILE *fp, char **pbuf, size_t *pbufsize) {
    char *buf = NULL;
    size_t bufsize;
    size_t nbytes;
    const size_t chunksize = 8192;

    bufsize = 0;
    buf = realloc(buf, chunksize);
    while ((nbytes = fread(buf + bufsize, 1, chunksize, fp)) == chunksize) {
        bufsize += chunksize;
        buf = realloc(buf, bufsize + chunksize);
    }

    bufsize += nbytes + 1;
    buf = realloc(buf, bufsize);
    buf[bufsize-1] = '\0';

    *pbuf = buf;
    *pbufsize = bufsize;

    return 0;
}

PyObject *
get_time_stamp(PyObject *self, PyObject *args) {
    struct timeval tv;
    PyObject *sec, *usec, *tup;

    gettimeofday(&tv, NULL);

    sec = PyLong_FromLong(tv.tv_sec);
    usec = PyLong_FromLong(tv.tv_usec);
    //return PyTuple_Pack(2, sec, usec);
    return Py_BuildValue("(OO)", sec, usec);
}

PyMethodDef tod_methods[] = {
    { "gettimestamp", get_time_stamp, METH_VARARGS, "Return a tuple with seconds and useconds since the epoch." },
    { NULL, NULL, 0, NULL }
};

int
main(int argc, char **argv) {
    int i;
    char *buf;
    size_t bufsize;
    PyObject *o, *globals, *locals;

    Py_Initialize();

    globals = PyDict_New();
    locals = PyDict_New();
    PyDict_SetItemString(globals, (char *)"__builtins__", PyEval_GetBuiltins());

    printf("globals = %p; locals = %p;\n\n", globals, locals);

    Py_InitModule("tod", tod_methods);

    for (i = 1; i < argc; ++i) {
        if (load_text_file(argv[i], &buf, &bufsize) == -1)
            continue;

        printf("### Executing file %s ###\n%s###################\n", argv[1], buf);

        if ((o = PyRun_String(buf, Py_file_input, globals, locals)) == NULL)
            printf("PyRun_String returned NULL\n");
        Py_XDECREF(o);

        printf("-------------------\n\n");

        free(buf);
    }

    Py_Finalize();

    return 0;
}

