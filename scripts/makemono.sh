make clean
rm -f hamlib-static.a

./configure --prefix=${HOME}/hl-static \
            --enable-static \
            --disable-shared \
            --enable-monolithic \
            --disable-winradio

make

objects="\
adat/*.o alinco/*.o aor/*.o \
drake/*.o dummy/*.o \
flexradio/*.o \
icom/*.o \
jrc/*.o \
kenwood/*.o \
lowe/*.o \
pcr/*.o prm80/*.o \
racal/*.o \
tentec/*.o \
yaesu/*.o \
c++/rigclass.o \
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

ar crvs hamlib-static.a $objects

#strip hamlib-static.a
mkdir -p ${HOME}/hl-static/lib/
cp hamlib-static.a ${HOME}/hl-static/lib/libhamlib.a
ls ${HOME}/hl-static/lib/libhamlib.a -l
