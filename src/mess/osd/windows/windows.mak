###########################################################################
#
#   windows.mak
#
#   MESS Windows-specific makefile
#
###########################################################################


CFLAGS += -DWINUI -DEMULATORDLL=\"$(EMULATORDLL)\"
RCFLAGS += -DMESS

LIBS += -lcomdlg32

OBJDIRS += \
	$(MESSOBJ)\osd \
	$(MESSOBJ)\osd\windows

MESS_WINSRC = src/mess/osd/windows
MESS_WINOBJ = $(OBJ)/mess/osd/windows

OSDOBJS += \
	$(MESS_WINOBJ)/configms.o	\
	$(MESS_WINOBJ)/dialog.o	\
	$(MESS_WINOBJ)/menu.o		\
	$(MESS_WINOBJ)/mess.res	\
	$(MESS_WINOBJ)/messlib.o	\
	$(MESS_WINOBJ)/opcntrl.o	\
	$(MESS_WINOBJ)/tapedlg.o

$(LIBOSD): $(OSDOBJS)

OSDCOREOBJS += \
	$(OBJ)/mess/osd/windows/winmess.o	\
	$(OBJ)/mess/osd/windows/winutils.o	\
	$(OBJ)/mess/osd/windows/glob.o

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOCORE_NOMAIN): $(OSDCOREOBJS:$(WINOBJ)/main.o=)



#-------------------------------------------------
# generic rules for the resource compiler
#-------------------------------------------------

$(MESS_WINOBJ)/%.res: $(MESS_WINSRC)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/$(MAMEOS) -o $@ -i $<

$(OBJ)/ui/%.res: src/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir src/ui -o $@ -i $<

$(OBJ)/mess/ui/%.res: mess/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/ui --include-dir src/ui --include-dir src -o $@ -i $<
