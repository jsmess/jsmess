#####################################################################
# make SUFFIX=32
#####################################################################

#-------------------------------------------------
# object and source roots
#-------------------------------------------------

WINUISRC = $(SRC)/osd/winui
WINUIOBJ = $(OBJ)/osd/winui

OBJDIRS += $(WINUIOBJ)
	
ifdef MESS
MESS_WINUISRC = $(SRC)/mess/osd/winui
MESS_WINUIOBJ = $(OBJ)/mess/osd/winui
OBJDIRS += $(MESS_WINUIOBJ)
CFLAGS += -I$(WINUISRC) -I$(MESS_WINUISRC)
endif



#-------------------------------------------------
# Windows UI object files
#-------------------------------------------------

WINUIOBJS += \
	$(WINUIOBJ)/m32util.o \
	$(WINUIOBJ)/directinput.o \
	$(WINUIOBJ)/dijoystick.o \
	$(WINUIOBJ)/directdraw.o \
	$(WINUIOBJ)/directories.o \
	$(WINUIOBJ)/audit32.o \
	$(WINUIOBJ)/columnedit.o \
	$(WINUIOBJ)/screenshot.o \
	$(WINUIOBJ)/treeview.o \
	$(WINUIOBJ)/splitters.o \
	$(WINUIOBJ)/bitmask.o \
	$(WINUIOBJ)/datamap.o \
	$(WINUIOBJ)/dxdecode.o \
	$(WINUIOBJ)/picker.o \
	$(WINUIOBJ)/properties.o \
	$(WINUIOBJ)/tabview.o \
	$(WINUIOBJ)/help.o \
	$(WINUIOBJ)/history.o \
	$(WINUIOBJ)/dialogs.o \
	$(WINUIOBJ)/dirwatch.o	\
	$(WINUIOBJ)/datafile.o	\
	$(WINUIOBJ)/m32opts.o \
	$(WINUIOBJ)/win32ui.o \
	$(WINUIOBJ)/helpids.o \

ifdef MESS
WINUIOBJS += \
	$(MESS_WINUIOBJ)/mess32ui.o \
	$(MESS_WINUIOBJ)/optionsms.o \
 	$(MESS_WINUIOBJ)/layoutms.o \
	$(MESS_WINUIOBJ)/ms32util.o \
	$(MESS_WINUIOBJ)/propertiesms.o \
	$(MESS_WINUIOBJ)/softwarepicker.o \
	$(MESS_WINUIOBJ)/devview.o
else
WINUIOBJS += \
 	$(WINUIOBJ)/layout.o \
	$(WINUIOBJ)/m32main.o \
	$(WINUIOBJ)/mame32.res
endif

ifdef MESS
# extra dependencies
# $(MESS_WINUIOBJ)/mess32ui.o:	$(WINUISRC)/win32ui.c $(MESS_WINUISRC)/mess32ui.c
# $(MESS_WINUIOBJ)/optionsms.o:	$(WINUISRC)/options.c $(MESS_WINUISRC)/optionsms.c
endif
	
# add resource file
ifdef MESS
GUIRESFILE = $(MESS_WINUIOBJ)/mess32.res
endif

$(LIBOSD): $(WINUIOBJS)


#-------------------------------------------------
# rules for creating helpids.c 
#-------------------------------------------------

$(WINUISRC)/helpids.c : $(WINUIOBJ)/mkhelp$(EXE) $(WINUISRC)/resource.h $(WINUISRC)/resource.hm $(WINUISRC)/mame32.rc
	$(WINUIOBJ)/mkhelp$(EXE) $(WINUISRC)/mame32.rc >$@

# rule to build the generator
$(WINUIOBJ)/mkhelp$(EXE): $(WINUIOBJ)/mkhelp.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@


#####################################################################
# compiler

#
# Preprocessor Definitions
#

ifdef MSVC_BUILD
DEFS += -DWINVER=0x0500 -D_CRT_NON_CONFORMING_SWPRINTFS
else
DEFS += -DWINVER=0x0400
endif

DEFS += \
	-D_WIN32_IE=0x0500 \
	-DDECL_SPEC= \
	-DZEXTERN=extern \

#	-DSHOW_UNAVAILABLE_FOLDER


#####################################################################
# Resources

ifndef MESS
UI_RC = @windres --use-temp-file

UI_RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

UI_RCFLAGS = -O coff --include-dir $(WINUISRC)

$(WINUIOBJ)/%.res: $(WINUISRC)/%.rc
	@echo Compiling mame32 resources $<...
	$(UI_RC) $(UI_RCDEFS) $(UI_RCFLAGS) -o $@ -i $<
endif

#####################################################################
# Linker

ifndef MSVC
LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \

endif

ifndef MESS
LDFLAGS += -mwindows
endif

#####################################################################

ifdef MESS

$(MESS_WINUIOBJ)/mess32.res:	$(WINUISRC)/mame32.rc $(MESS_WINUISRC)/mess32.rc $(WINUISRC)/resource.h $(MESS_WINUISRC)/resourcems.h $(WINUIOBJ)/mamevers.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WINUISRC) --include-dir $(MESS_WINUISRC) --include-dir $(WINUIOBJ) -o $@ -i $(MESS_WINUISRC)/mess32.rc

$(WINUIOBJ)/mamevers.rc: $(WINOBJ)/verinfo$(EXE) $(SRC)/version.c
	@echo Emitting $@...
	@$(VERINFO) $(SRC)/version.c > $@

endif
