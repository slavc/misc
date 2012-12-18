#!/bin/sh

WLD=$HOME/opt
LD_LIBRARY_PATH=$WLD/lib
PKG_CONFIG_PATH=$WLD/lib/pkgconfig/:$WLD/share/pkgconfig/
ACLOCAL_DIR=$WLD/share/aclocal
ACLOCAL="aclocal -I $ACLOCAL_DIR"

mkdir -p $ACLOCAL_DIR

export WLD LD_LIBRARY_PATH PKG_CONFIG_PATH ACLOCAL
