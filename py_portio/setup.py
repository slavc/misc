import os
from distutils.core import setup, Extension

(sysname, nodename, release, version, machine) = os.uname()
macros = [("OS_%s"%sysname.upper(), "1"), ("ARCH_%s"%machine.upper(), "1")]
if sysname == "OpenBSD":
	libs = [machine] # to use iopl() on OBSD, you must compile with -li386 or -lamd64
else:
	libs = []

module1 = Extension('portio',
					libraries = libs,
					define_macros = macros,
					sources = ['portio.c'])

setup (name = 'portio',
       version = '0.1',
       description = 'A module for accessing I/O ports on AMD64 and i386 on UNIXen.',
       ext_modules = [module1])
