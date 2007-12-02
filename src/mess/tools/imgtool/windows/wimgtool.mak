###########################################################################
#
#   wimgtool.mak
#
#   MESS wimgtool makefile
#
###########################################################################


# wimgtool executable name
WIMGTOOL = wimgtool$(EXE)

# wimgtool directories
WIMGTOOLSRC = $(MESSSRC)/tools/imgtool/windows
WIMGTOOLOBJ = $(MESSOBJ)/tools/imgtool/windows



#-------------------------------------------------
# wimgtool objects
#-------------------------------------------------

OBJDIRS += $(WIMGTOOLOBJ)

WIMGTOOL_OBJS = \
	$(MESSOBJ)/pile.o								\
	$(MESSOBJ)/toolerr.o							\
	$(MESSOBJ)/osd/windows/opcntrl.o				\
	$(MESSOBJ)/osd/windows/winutils.o				\
	$(MESSOBJ)/tools/imgtool/stubs.o				\
	$(MESSOBJ)/tools/imgtool/windows/wmain.o		\
	$(MESSOBJ)/tools/imgtool/windows/wimgtool.o		\
	$(MESSOBJ)/tools/imgtool/windows/attrdlg.o		\
	$(MESSOBJ)/tools/imgtool/windows/assoc.o		\
	$(MESSOBJ)/tools/imgtool/windows/assocdlg.o		\
	$(MESSOBJ)/tools/imgtool/windows/hexview.o		\
	$(MESSOBJ)/tools/imgtool/windows/secview.o		\
	$(MESSOBJ)/tools/imgtool/windows/wimgtool.res	\



#-------------------------------------------------
# rules to build the wimgtool executable
#-------------------------------------------------

$(WIMGTOOLOBJ)/%.res: $(WIMGTOOLSRC)/%.rc | $(OSPREBUILD)
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir $(WIMGTOOLSRC) -o $@ -i $<

$(WIMGTOOL): $(WIMGTOOL_OBJS) $(LIBIMGTOOL) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE_NOMAIN)
	@echo Linking $@...
	$(LD) $(LDFLAGS) -mwindows $^ $(LIBS) -o $@
