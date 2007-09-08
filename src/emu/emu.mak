###########################################################################
#
#   emu.mak
#
#   MAME emulation core makefile
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


EMUSRC = $(SRC)/emu
EMUOBJ = $(OBJ)/emu

OBJDIRS += \
	$(EMUOBJ)/cpu \
	$(EMUOBJ)/sound \
	$(EMUOBJ)/debug \
	$(EMUOBJ)/audio \
	$(EMUOBJ)/drivers \
	$(EMUOBJ)/machine \
	$(EMUOBJ)/layout \
	$(EMUOBJ)/video \



#-------------------------------------------------
# emulator core objects
#-------------------------------------------------

EMUOBJS = \
	$(EMUOBJ)/audit.o \
	$(EMUOBJ)/cheat.o \
	$(EMUOBJ)/clifront.o \
	$(EMUOBJ)/config.o \
	$(EMUOBJ)/cpuexec.o \
	$(EMUOBJ)/cpuint.o \
	$(EMUOBJ)/cpuintrf.o \
	$(EMUOBJ)/drawgfx.o \
	$(EMUOBJ)/driver.o \
	$(EMUOBJ)/emuopts.o \
	$(EMUOBJ)/emupal.o \
	$(EMUOBJ)/fileio.o \
	$(EMUOBJ)/hash.o \
	$(EMUOBJ)/info.o \
	$(EMUOBJ)/input.o \
	$(EMUOBJ)/inputseq.o \
	$(EMUOBJ)/inptport.o \
	$(EMUOBJ)/mame.o \
	$(EMUOBJ)/mamecore.o \
	$(EMUOBJ)/memory.o \
	$(EMUOBJ)/output.o \
	$(EMUOBJ)/render.o \
	$(EMUOBJ)/rendfont.o \
	$(EMUOBJ)/rendlay.o \
	$(EMUOBJ)/rendutil.o \
	$(EMUOBJ)/restrack.o \
	$(EMUOBJ)/romload.o \
	$(EMUOBJ)/sound.o \
	$(EMUOBJ)/sndintrf.o \
	$(EMUOBJ)/state.o \
	$(EMUOBJ)/streams.o \
	$(EMUOBJ)/tilemap.o \
	$(EMUOBJ)/timer.o \
	$(EMUOBJ)/ui.o \
	$(EMUOBJ)/uigfx.o \
	$(EMUOBJ)/uimenu.o \
	$(EMUOBJ)/uitext.o \
	$(EMUOBJ)/validity.o \
	$(EMUOBJ)/video.o \
	$(EMUOBJ)/sound/filter.o \
	$(EMUOBJ)/sound/flt_vol.o \
	$(EMUOBJ)/sound/flt_rc.o \
	$(EMUOBJ)/sound/wavwrite.o \
	$(EMUOBJ)/audio/generic.o \
	$(EMUOBJ)/drivers/empty.o \
	$(EMUOBJ)/machine/eeprom.o \
	$(EMUOBJ)/machine/generic.o \
	$(EMUOBJ)/video/generic.o \
	$(EMUOBJ)/video/resnet.o \
	$(EMUOBJ)/video/vector.o \

ifdef DEBUG
EMUOBJS += \
	$(EMUOBJ)/profiler.o \
	$(EMUOBJ)/debug/debugcmd.o \
	$(EMUOBJ)/debug/debugcmt.o \
	$(EMUOBJ)/debug/debugcon.o \
	$(EMUOBJ)/debug/debugcpu.o \
	$(EMUOBJ)/debug/debughlp.o \
	$(EMUOBJ)/debug/debugvw.o \
	$(EMUOBJ)/debug/express.o \
	$(EMUOBJ)/debug/textbuf.o
endif

$(LIBEMU): $(EMUOBJS)



#-------------------------------------------------
# CPU core objects
#-------------------------------------------------

include $(EMUSRC)/cpu/cpu.mak

$(LIBCPU): $(CPUOBJS)

ifdef DEBUG
$(LIBCPU): $(DBGOBJS)
endif



#-------------------------------------------------
# sound core objects
#-------------------------------------------------

include $(EMUSRC)/sound/sound.mak

$(LIBSOUND): $(SOUNDOBJS)



#-------------------------------------------------
# additional dependencies
#-------------------------------------------------

$(EMUOBJ)/rendfont.o:	$(EMUOBJ)/uismall.fh

$(EMUOBJ)/video.o:		$(EMUSRC)/rendersw.c



#-------------------------------------------------
# core layouts
#-------------------------------------------------

$(EMUOBJ)/rendlay.o:	$(EMUOBJ)/layout/dualhovu.lh \
						$(EMUOBJ)/layout/dualhsxs.lh \
						$(EMUOBJ)/layout/dualhuov.lh \
						$(EMUOBJ)/layout/horizont.lh \
						$(EMUOBJ)/layout/triphsxs.lh \
						$(EMUOBJ)/layout/vertical.lh \
						$(EMUOBJ)/layout/ho20ffff.lh \
						$(EMUOBJ)/layout/ho2eff2e.lh \
						$(EMUOBJ)/layout/ho4f893d.lh \
						$(EMUOBJ)/layout/ho88ffff.lh \
						$(EMUOBJ)/layout/hoa0a0ff.lh \
						$(EMUOBJ)/layout/hoffe457.lh \
						$(EMUOBJ)/layout/hoffff20.lh \
						$(EMUOBJ)/layout/voffff20.lh \

$(EMUOBJ)/video.o:		$(EMUOBJ)/layout/snap.lh
