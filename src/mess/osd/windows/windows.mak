###########################################################################
#
#   windows.mak
#
#   MESS Windows-specific makefile
#
###########################################################################


# build the executable names
FULLGUINAME = $(PREFIX)$(NAME)ui$(SUFFIX)$(DEBUGSUFFIX)
EMULATORCLI = $(FULLNAME)$(EXE)
EMULATORGUI = $(FULLGUINAME)$(EXE)
EMULATORDLL = $(FULLNAME)lib.dll
EMULATOR += $(EMULATORDLL) $(EMULATORGUI)

CFLAGS += -DWINUI -DEMULATORDLL=\"$(EMULATORDLL)\"
RCFLAGS += -DMESS

LIBS += -lcomdlg32

OBJDIRS += \
	$(MESSOBJ)/osd \
	$(MESSOBJ)/osd/windows

MESS_WINSRC = src/mess/osd/windows
MESS_WINOBJ = $(OBJ)/mess/osd/windows

OSDOBJS += \
	$(MESS_WINOBJ)/configms.o	\
	$(MESS_WINOBJ)/dialog.o	\
	$(MESS_WINOBJ)/menu.o		\
	$(MESS_WINOBJ)/mess.res	\
	$(MESS_WINOBJ)/messlib.o	\
	$(MESS_WINOBJ)/opcntrl.o

$(LIBOSD): $(OSDOBJS)

OSDCOREOBJS += \
	$(OBJ)/mess/osd/windows/winmess.o	\
	$(OBJ)/mess/osd/windows/winutils.o	\
	$(OBJ)/mess/osd/windows/glob.o

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)


ifdef MSVC_BUILD
DLLLINK = lib
else
DLLLINK = dll
endif



#-------------------------------------------------
# if building with a UI, include the ui.mak
#-------------------------------------------------

include $(SRC)/mess/osd/winui/winui.mak



#-------------------------------------------------
# generic rules for the resource compiler
#-------------------------------------------------

$(MESS_WINOBJ)/%.res: $(MESS_WINSRC)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/$(OSD) -o $@ -i $<



#-------------------------------------------------
# executable targets
#-------------------------------------------------

EXECUTABLE_DEFINED = 1

$(EMULATORDLL): $(VERSIONOBJ) $(OBJ)/mess/osd/windows/messlib.o $(DRVLIBS) $(LIBOSD) $(LIBEMU) $(LIBCPU) $(LIBDASM) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE) $(MESS_WINOBJ)/mess.res
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c $(SRC)/version.c -o $(VERSIONOBJ)
	@echo Linking $@...
	$(LD) -shared $(LDFLAGS) $(LDFLAGSEMULATOR) $^ $(LIBS) -o $@

# gui target
$(EMULATORGUI):	$(EMULATORDLL) $(OBJ)/mess/osd/winui/guimain.o $(GUIRESFILE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(LDFLAGSEMULATOR) -mwindows $(FULLNAME)lib.$(DLLLINK) $(OBJ)/mess/osd/winui/guimain.o $(GUIRESFILE) $(LIBS) -o $@

# cli target
$(EMULATORCLI):	$(EMULATORDLL) $(OBJ)/mess/osd/windows/climain.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(LDFLAGSEMULATOR) -mconsole $(FULLNAME)lib.$(DLLLINK) $(OBJ)/mess/osd/windows/climain.o $(LIBS) -o $@
