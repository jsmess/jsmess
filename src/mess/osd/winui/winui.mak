###########################################################################
#
#   winui.mak
#
#   MESS Windows-specific makefile
#
###########################################################################

# build the executable names
RCFLAGS += -DMESS

EMU_EXE = $(PREFIX)$(NAME)$(SUFFIX)$(SUFFIX64)$(SUFFIXDEBUG)$(EXE)
BUILD += $(EMU_EXE)

MESS_WINUISRC = $(SRC)/mess/osd/winui
MESS_WINUIOBJ = $(OBJ)/mess/osd/winui
WINUISRC = $(SRC)/osd/winui
WINUIOBJ = $(OBJ)/osd/winui

RESFILE = $(MESS_WINUIOBJ)/messui.res

CFLAGS += \
	-I$(MESSSRC)/osd/winui

OBJDIRS += \
	$(MESSOBJ)/osd \
	$(MESSOBJ)/osd/winui


$(MESS_WINUIOBJ)/messui.res:	$(WINUISRC)/mameui.rc $(MESS_WINUISRC)/messui.rc $(WINUISRC)/resource.h $(WINUISRC)/resourcems.h $(WINUIOBJ)/mamevers.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WINUISRC) --include-dir $(MESS_WINUISRC) --include-dir $(WINUIOBJ) -o $@ -i $(MESS_WINUISRC)/messui.rc

$(WINUIOBJ)/mamevers.rc: $(OBJ)/build/verinfo$(EXE) $(SRC)/version.c
	@echo Emitting $@...
	@"$(VERINFO)" -b mess $(SRC)/version.c  > $@

$(LIBOSD): $(OSDOBJS)

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)

#-------------------------------------------------
# OSD Windows library
#-------------------------------------------------

WINOSDOBJS = \
	$(WINOBJ)/d3d9intf.o \
	$(WINOBJ)/drawd3d.o \
	$(WINOBJ)/drawdd.o \
	$(WINOBJ)/drawgdi.o \
	$(WINOBJ)/drawnone.o \
	$(WINOBJ)/input.o \
	$(WINOBJ)/output.o \
	$(WINOBJ)/sound.o \
	$(WINOBJ)/video.o \
	$(WINOBJ)/window.o \
	$(WINOBJ)/winmain.o	\
	$(WINOBJ)/debugwin.o \
	$(UIOBJ)/dialog.o	\
	$(UIOBJ)/menu.o	\
	$(UIOBJ)/opcntrl.o	\
	$(UIOBJ)/winutils.o

ifeq ($(DIRECT3D),8)
WINOSDOBJS += $(WINOBJ)/d3d8intf.o
endif

$(EMU_EXE): $(VERSIONOBJ) $(DRVLIBS) $(WINOSDOBJS) $(LIBCPU) $(LIBEMU) $(LIBDASM) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(SOFTFLOAT) $(LIBOCORE) $(RESFILE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
