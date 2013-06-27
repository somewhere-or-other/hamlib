make clean
rm -f hamlib-win32.a

./configure --prefix=${HOME}/hl-win32-static \
            --build=i686-pc-linux-gnu --host=i686-pc-mingw32 \
            --enable-static \
            --without-c++-binding \
            --disable-winradio \
            --enable-monolithic

make

objects="\
adat/*.o alinco/*.o aor/*.o \
drake/*.o dummy/*.o \
flexradio/*.o \
icom/*.o \
jrc/*.o \
kenwood/*.o \
lib/*.o \
lowe/*.o \
pcr/*.o prm80/*.o \
racal/*.o \
tentec/*.o \
yaesu/*.o \
src/libhamlib_la-cal.o src/libhamlib_la-cm108.o \
src/libhamlib_la-conf.o src/libhamlib_la-debug.o \
src/libhamlib_la-event.o src/libhamlib_la-ext.o \
src/libhamlib_la-iofunc.o src/libhamlib_la-locator.o \
src/libhamlib_la-mem.o src/libhamlib_la-misc.o \
src/libhamlib_la-network.o src/libhamlib_la-parallel.o \
src/libhamlib_la-register.o src/libhamlib_la-rig.o \
src/libhamlib_la-serial.o src/libhamlib_la-settings.o \
src/libhamlib_la-tones.o \
"
echo $objects

i686-pc-mingw32-ar crvs hamlib-win32.a $objects
i686-pc-mingw32-ranlib hamlib-win32.a
i686-pc-mingw32-strip hamlib-win32.a

mkdir -p ${HOME}/hl-win32-static/lib/

cp hamlib-win32.a ${HOME}/hl-win32-static/lib/libhamlib.a
ls ${HOME}/hl-win32-static/lib/libhamlib.a -l

cp hamlib-win32.a /opt/mxe/usr/i686-pc-mingw32/lib/libhamlib.a
ls /opt/mxe/usr/i686-pc-mingw32/lib/libhamlib.a -l
