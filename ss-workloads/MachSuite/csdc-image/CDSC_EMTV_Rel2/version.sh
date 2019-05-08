#!/bin/sh
VER=`svnversion .`
if [ "x$VER" != x -a "$VER" != exported ]
then
  echo "#define LSLS_VERSION \" svn-$VER\"" >> config.h
  VER=`echo $VER | sed -e 's/[^0-9].*//'`
else
  echo "#define LSLS_VERSION \"\"" >> config.h
  VER="x"
fi
API=`grep '#define LSLS_BUILD' < config.h | sed -e 's/.* \([1-9][0-9]*\).*/\1/'`
echo "#define LSLS_POINTVER \"0.$API.$VER\"" >> config.h
