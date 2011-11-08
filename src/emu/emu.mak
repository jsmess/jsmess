###########################################################################
#
#   emu.mak
#
#   MAME emulation core makefile
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


EMUSRC = $(SRC)/emu
EMUOBJ = $(OBJ)/emu

EMUAUDIO = $(EMUOBJ)/audio
EMUDRIVERS = $(EMUOBJ)/drivers
EMULAYOUT = $(EMUOBJ)/layout
EMUMACHINE = $(EMUOBJ)/machine
EMUIMAGEDEV = $(EMUOBJ)/imagedev
EMUVIDEO = $(EMUOBJ)/video

OBJDIRS += \
	$(EMUOBJ)/cpu \
	$(EMUOBJ)/sound \
	$(EMUOBJ)/debug \
	$(EMUOBJ)/debugint \
	$(EMUOBJ)/audio \
	$(EMUOBJ)/drivers \
	$(EMUOBJ)/machine \
	$(EMUOBJ)/layout \
	$(EMUOBJ)/imagedev \
	$(EMUOBJ)/video \

OSDSRC = $(SRC)/osd
OSDOBJ = $(OBJ)/osd

OBJDIRS += \
	$(OSDOBJ)


#-------------------------------------------------
# emulator core objects
#-------------------------------------------------

EMUOBJS = \
	$(EMUOBJ)/hashfile.o \
	$(EMUOBJ)/addrmap.o \
	$(EMUOBJ)/attotime.o \
	$(EMUOBJ)/audit.o \
	$(EMUOBJ)/cheat.o \
	$(EMUOBJ)/clifront.o \
	$(EMUOBJ)/config.o \
	$(EMUOBJ)/crsshair.o \
	$(EMUOBJ)/debugger.o \
	$(EMUOBJ)/delegate.o \
	$(EMUOBJ)/devcb.o \
	$(EMUOBJ)/devcpu.o \
	$(EMUOBJ)/device.o \
	$(EMUOBJ)/devlegcy.o \
	$(EMUOBJ)/didisasm.o \
	$(EMUOBJ)/diexec.o \
	$(EMUOBJ)/diimage.o \
	$(EMUOBJ)/dimemory.o \
	$(EMUOBJ)/dinetwork.o \
	$(EMUOBJ)/dinvram.o \
	$(EMUOBJ)/dirtc.o \
	$(EMUOBJ)/diserial.o \
	$(EMUOBJ)/dislot.o \
	$(EMUOBJ)/disound.o \
	$(EMUOBJ)/distate.o \
	$(EMUOBJ)/drawgfx.o \
	$(EMUOBJ)/driver.o \
	$(EMUOBJ)/drivenum.o \
	$(EMUOBJ)/emualloc.o \
	$(EMUOBJ)/emucore.o \
	$(EMUOBJ)/emuopts.o \
	$(EMUOBJ)/emupal.o \
	$(EMUOBJ)/fileio.o \
	$(EMUOBJ)/hash.o \
	$(EMUOBJ)/image.o \
	$(EMUOBJ)/info.o \
	$(EMUOBJ)/input.o \
	$(EMUOBJ)/ioport.o \
	$(EMUOBJ)/mame.o \
	$(EMUOBJ)/machine.o \
	$(EMUOBJ)/mconfig.o \
	$(EMUOBJ)/memory.o \
	$(EMUOBJ)/network.o \
	$(EMUOBJ)/output.o \
	$(EMUOBJ)/render.o \
	$(EMUOBJ)/rendfont.o \
	$(EMUOBJ)/rendlay.o \
	$(EMUOBJ)/rendutil.o \
	$(EMUOBJ)/romload.o \
	$(EMUOBJ)/save.o \
	$(EMUOBJ)/schedule.o \
	$(EMUOBJ)/screen.o \
	$(EMUOBJ)/softlist.o \
	$(EMUOBJ)/sound.o \
	$(EMUOBJ)/speaker.o \
	$(EMUOBJ)/tilemap.o \
	$(EMUOBJ)/timer.o \
	$(EMUOBJ)/ui.o \
	$(EMUOBJ)/uigfx.o \
	$(EMUOBJ)/uiimage.o \
	$(EMUOBJ)/uiinput.o \
	$(EMUOBJ)/uiswlist.o \
	$(EMUOBJ)/uimain.o \
	$(EMUOBJ)/uimenu.o \
	$(EMUOBJ)/validity.o \
	$(EMUOBJ)/video.o \
	$(EMUOBJ)/debug/debugcmd.o \
	$(EMUOBJ)/debug/debugcon.o \
	$(EMUOBJ)/debug/debugcpu.o \
	$(EMUOBJ)/debug/debughlp.o \
	$(EMUOBJ)/debug/debugvw.o \
	$(EMUOBJ)/debug/dvdisasm.o \
	$(EMUOBJ)/debug/dvmemory.o \
	$(EMUOBJ)/debug/dvstate.o \
	$(EMUOBJ)/debug/dvtext.o \
	$(EMUOBJ)/debug/express.o \
	$(EMUOBJ)/debug/textbuf.o \
	$(EMUOBJ)/debugint/debugint.o \
	$(EMUOBJ)/profiler.o \
	$(OSDOBJ)/osdepend.o \
	$(OSDOBJ)/osdnet.o

EMUSOUNDOBJS = \
	$(EMUOBJ)/sound/filter.o \
	$(EMUOBJ)/sound/flt_vol.o \
	$(EMUOBJ)/sound/flt_rc.o \
	$(EMUOBJ)/sound/wavwrite.o \

EMUAUDIOOBJS = \

EMUDRIVEROBJS = \
	$(EMUDRIVERS)/empty.o \
	$(EMUDRIVERS)/testcpu.o \

EMUMACHINEOBJS = \
	$(EMUMACHINE)/8255ppi.o		\
	$(EMUMACHINE)/generic.o		\
	$(EMUMACHINE)/ldstub.o		\
	$(EMUMACHINE)/ldpr8210.o	\
	$(EMUMACHINE)/ldv1000.o		\
	$(EMUMACHINE)/ldvp931.o		\
	$(EMUMACHINE)/ram.o			\
	$(EMUMACHINE)/z80ctc.o		\

EMUVIDEOOBJS = \
	$(EMUVIDEO)/generic.o		\
	$(EMUVIDEO)/rgbutil.o		\
	$(EMUVIDEO)/tms9928a.o		\
	$(EMUVIDEO)/vector.o		\

EMUIMAGEDEVOBJS = \
	$(EMUIMAGEDEV)/bitbngr.o	\
	$(EMUIMAGEDEV)/cartslot.o	\
	$(EMUIMAGEDEV)/cassette.o	\
	$(EMUIMAGEDEV)/chd_cd.o		\
	$(EMUIMAGEDEV)/flopdrv.o	\
	$(EMUIMAGEDEV)/floppy.o		\
	$(EMUIMAGEDEV)/harddriv.o	\
	$(EMUIMAGEDEV)/printer.o	\
	$(EMUIMAGEDEV)/serial.o		\
	$(EMUIMAGEDEV)/snapquik.o	\


LIBEMUOBJS = $(EMUOBJS) $(EMUSOUNDOBJS) $(EMUAUDIOOBJS) $(EMUDRIVEROBJS) $(EMUMACHINEOBJS) $(EMUIMAGEDEVOBJS) $(EMUVIDEOOBJS)

$(LIBEMU): $(LIBEMUOBJS)



#-------------------------------------------------
# CPU core objects
#-------------------------------------------------

include $(EMUSRC)/cpu/cpu.mak

$(LIBCPU): $(CPUOBJS)

$(LIBDASM): $(DASMOBJS)



#-------------------------------------------------
# sound core objects
#-------------------------------------------------

include $(EMUSRC)/sound/sound.mak

$(LIBSOUND): $(SOUNDOBJS)



#-------------------------------------------------
# additional dependencies
#-------------------------------------------------

$(EMUOBJ)/rendfont.o:	$(EMUOBJ)/uismall.fh

$(EMUOBJ)/video.o:	$(EMUSRC)/rendersw.c

$(EMUMACHINE)/s3c2400.o:	$(EMUSRC)/machine/s3c24xx.c
$(EMUMACHINE)/s3c2410.o:	$(EMUSRC)/machine/s3c24xx.c
$(EMUMACHINE)/s3c2440.o:	$(EMUSRC)/machine/s3c24xx.c


#-------------------------------------------------
# core layouts
#-------------------------------------------------

$(EMUOBJ)/rendlay.o:	$(EMULAYOUT)/dualhovu.lh \
						$(EMULAYOUT)/dualhsxs.lh \
						$(EMULAYOUT)/dualhuov.lh \
						$(EMULAYOUT)/horizont.lh \
						$(EMULAYOUT)/triphsxs.lh \
						$(EMULAYOUT)/quadhsxs.lh \
						$(EMULAYOUT)/vertical.lh \
						$(EMULAYOUT)/ho20ffff.lh \
						$(EMULAYOUT)/ho2eff2e.lh \
						$(EMULAYOUT)/ho4f893d.lh \
						$(EMULAYOUT)/ho88ffff.lh \
						$(EMULAYOUT)/hoa0a0ff.lh \
						$(EMULAYOUT)/hoffe457.lh \
						$(EMULAYOUT)/hoffff20.lh \
						$(EMULAYOUT)/voffff20.lh \
						$(EMULAYOUT)/lcd.lh \
						$(EMULAYOUT)/lcd_rot.lh \
						$(EMULAYOUT)/noscreens.lh \

$(EMUOBJ)/video.o:		$(EMULAYOUT)/snap.lh
