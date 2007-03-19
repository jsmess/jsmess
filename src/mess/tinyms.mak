###########################################################################
#
#   tinyms.mak
#
#   Small driver-specific example makefile
#   Use make -f makefile.mes TARGET=tinyms to build
#
###########################################################################

MESS = 1
COREDEFS += -DTINY_COMPILE=1 -DMESS



#-------------------------------------------------
# You need to define two strings:
#
#	TINY_NAME is a comma-separated list of driver
#	names that will be referenced.
#
#	TINY_DRIVER should be the same list but with
#	an & in front of each name.
#-------------------------------------------------

COREDEFS += -DTINY_NAME="driver_coleco"
COREDEFS += -DTINY_POINTER="&driver_coleco"



#-------------------------------------------------
# Specify all the CPU cores necessary for these
# drivers.
#-------------------------------------------------

CPUS += Z80



#-------------------------------------------------
# Specify all the sound cores necessary for these
# drivers.
#-------------------------------------------------

SOUNDS += SN76496



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# above.
#-------------------------------------------------

OBJS =\
	$(OBJ)/mess/machine/coleco.o	 \
	$(OBJ)/mess/systems/coleco.o



#-------------------------------------------------
# MESS specific core $(OBJ)s
#-------------------------------------------------

COREOBJS +=							\
	$(EXPAT)						\
	$(ZLIB)							\
	$(OBJ)/vidhrdw/tms9928a.o		\
	$(OBJ)/vidhrdw/v9938.o          \
	$(OBJ)/machine/8255ppi.o		\
	$(OBJ)/machine/6522via.o		\
	$(OBJ)/machine/6821pia.o		\
	$(OBJ)/machine/z80ctc.o			\
	$(OBJ)/machine/z80pio.o			\
	$(OBJ)/machine/z80sio.o			\
	$(OBJ)/machine/idectrl.o		\
	$(OBJ)/machine/6532riot.o		\
	$(OBJ)/mess/mess.o				\
	$(OBJ)/mess/mesvalid.o			\
	$(OBJ)/mess/image.o				\
	$(OBJ)/mess/messdriv.o			\
	$(OBJ)/mess/device.o			\
	$(OBJ)/mess/hashfile.o			\
	$(OBJ)/mess/inputx.o			\
	$(OBJ)/mess/artworkx.o			\
	$(OBJ)/mess/uimess.o			\
	$(OBJ)/mess/filemngr.o			\
	$(OBJ)/mess/tapectrl.o			\
	$(OBJ)/mess/compcfg.o			\
	$(OBJ)/mess/utils.o				\
	$(OBJ)/mess/eventlst.o			\
	$(OBJ)/mess/mscommon.o			\
	$(OBJ)/mess/pool.o				\
	$(OBJ)/mess/cheatms.o			\
	$(OBJ)/mess/opresolv.o			\
	$(OBJ)/mess/muitext.o			\
	$(OBJ)/mess/infomess.o			\
	$(OBJ)/mess/formats/ioprocs.o	\
	$(OBJ)/mess/formats/flopimg.o	\
	$(OBJ)/mess/formats/cassimg.o	\
	$(OBJ)/mess/formats/basicdsk.o	\
	$(OBJ)/mess/formats/pc_dsk.o	\
	$(OBJ)/mess/devices/mflopimg.o	\
	$(OBJ)/mess/devices/cassette.o	\
	$(OBJ)/mess/devices/cartslot.o	\
	$(OBJ)/mess/devices/printer.o	\
	$(OBJ)/mess/devices/bitbngr.o	\
	$(OBJ)/mess/devices/snapquik.o	\
	$(OBJ)/mess/devices/basicdsk.o	\
	$(OBJ)/mess/devices/flopdrv.o	\
	$(OBJ)/mess/devices/harddriv.o	\
	$(OBJ)/mess/devices/idedrive.o	\
	$(OBJ)/mess/devices/dsk.o		\
	$(OBJ)/mess/devices/z80bin.o	\
	$(OBJ)/mess/devices/chd_cd.o	\
	$(OBJ)/mess/machine/6551.o		\
	$(OBJ)/mess/machine/smartmed.o	\
	$(OBJ)/mess/vidhrdw/m6847.o		\
	$(OBJ)/mess/vidhrdw/m6845.o		\
	$(OBJ)/mess/machine/msm8251.o  \
	$(OBJ)/mess/machine/tc8521.o   \
	$(OBJ)/mess/vidhrdw/crtc6845.o \
	$(OBJ)/mess/machine/28f008sa.o \
	$(OBJ)/mess/machine/am29f080.o \
	$(OBJ)/mess/machine/rriot.o    \
	$(OBJ)/mess/machine/riot6532.o \
	$(OBJ)/machine/pit8253.o  \
	$(OBJ)/machine/mc146818.o \
	$(OBJ)/mess/machine/uart8250.o \
	$(OBJ)/mess/machine/pc_mouse.o \
	$(OBJ)/mess/machine/pclpt.o    \
	$(OBJ)/mess/machine/centroni.o \
	$(OBJ)/machine/pckeybrd.o \
	$(OBJ)/mess/machine/d88.o      \
	$(OBJ)/mess/machine/nec765.o   \
	$(OBJ)/mess/machine/wd17xx.o   \
	$(OBJ)/mess/machine/serial.o   \
	$(OBJ)/mess/formats/wavfile.o
