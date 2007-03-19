#-------------------------------------------------
# wimgtool
#-------------------------------------------------

WIMGTOOLSRC = $(SRC)/mess/tools/imgtool/windows
WIMGTOOLOBJ = $(OBJ)/mess/tools/imgtool/windows

OBJDIRS += $(WIMGTOOLOBJ)

WIMGTOOL_OBJS=\
	$(IMGTOOL_LIB_OBJS)								\
	$(OBJ)/mess/pile.o								\
	$(OBJ)/mess/toolerr.o							\
	$(OBJ)/mess/osd/windows/opcntrl.o				\
	$(OBJ)/mess/osd/windows/winutils.o				\
	$(OBJ)/mess/tools/imgtool/stubs.o				\
	$(OBJ)/mess/tools/imgtool/windows/wmain.o		\
	$(OBJ)/mess/tools/imgtool/windows/wimgtool.o	\
	$(OBJ)/mess/tools/imgtool/windows/attrdlg.o		\
	$(OBJ)/mess/tools/imgtool/windows/assoc.o		\
	$(OBJ)/mess/tools/imgtool/windows/assocdlg.o	\
	$(OBJ)/mess/tools/imgtool/windows/hexview.o		\
	$(OBJ)/mess/tools/imgtool/windows/secview.o		\
	$(OBJ)/mess/tools/imgtool/windows/wimgtool.res	\

$(OBJ)/mess/tools/imgtool/$(MAMEOS)/%.res: mess/tools/imgtool/$(MAMEOS)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/tools/imgtool/$(MAMEOS) -o $@ -i $<
	
wimgtool$(EXE):	 $(WIMGTOOL_OBJS) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE_NOMAIN)
	@echo Linking $@...
	$(LD) $(LDFLAGS) -mwindows $^ $(LIBS) -o $@



#-------------------------------------------------
# generic rule for the resource compiler
#-------------------------------------------------

$(WIMGTOOLOBJ)/%.res: $(WIMGTOOLSRC)/%.rc | $(OSPREBUILD)
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WIMGTOOLSRC) -o $@ -i $<
