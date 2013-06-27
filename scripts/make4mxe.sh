#!/bin/bash
#=======================================================================
# make the mxe-mingw32 executable

./configure \
  $PKGCFG \
  $CROSSCFG \
  --without-asciidoc \
  --with-ptw32=/opt/mxe/usr/i686-pc-mingw32 \
  --with-libiconv-prefix=$PREFIX/iconv \
  --enable-static \
  --with-libintl-prefix=$PREFIX/gettext \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre -lregex" \
  FLTK_CONFIG=$PREFIX/bin/i686-pc-mingw32-fltk-config \

make

$PREFIX/bin/i686-pc-mingw32-strip src/fldigi.exe
$PREFIX/bin/i686-pc-mingw32-strip src/flarq.exe

ls src/fldigi.exe -l
ls src/flarq.exe -l

make nsisinst

cp src/*setup.exe .

ls fldigi*.exe -l
