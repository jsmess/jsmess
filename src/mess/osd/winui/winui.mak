###########################################################################
#
#   winui.mak
#
#   MESS Windows-specific makefile
#
###########################################################################

# build the executable names
RCFLAGS += -DMESS
DEFS += -DMENU_BAR=1

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

OSDOBJS += \
	$(MESS_WINUIOBJ)/messui.o \
	$(MESS_WINUIOBJ)/optionsms.o \
	$(MESS_WINUIOBJ)/layoutms.o \
	$(MESS_WINUIOBJ)/msuiutil.o \
	$(MESS_WINUIOBJ)/propertiesms.o \
	$(MESS_WINUIOBJ)/swconfig.o \
	$(MESS_WINUIOBJ)/softwarepicker.o \
	$(MESS_WINUIOBJ)/softwarelist.o \
	$(MESS_WINUIOBJ)/devview.o	\
	$(MESS_WINUIOBJ)/dialog.o	\
	$(MESS_WINUIOBJ)/menu.o	\
	$(MESS_WINUIOBJ)/opcntrl.o	\
	$(MESS_WINUIOBJ)/winutils.o

$(MESS_WINUIOBJ)/messui.res:	$(WINUISRC)/mameui.rc $(MESS_WINUISRC)/messui.rc $(WINUISRC)/resource.h $(MESS_WINUISRC)/resourcems.h $(WINUIOBJ)/mamevers.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WINUISRC) --include-dir $(MESS_WINUISRC) --include-dir $(WINUIOBJ) -o $@ -i $(MESS_WINUISRC)/messui.rc

$(WINUIOBJ)/mamevers.rc: $(OBJ)/build/verinfo$(EXE) $(SRC)/version.c
	@echo Emitting $@...
	@"$(VERINFO)" -b mess $(SRC)/version.c  > $@
	
$(LIBOSD): $(OSDOBJS)

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)
