###########################################################################
#
#   messtest.mak
#
#   MESS messtest makefile
#
###########################################################################


# messtest executable name
MESSTEST = messtest$(EXE)

# messtest directories
OBJDIRS += $(MESSOBJ)/tools/messtest



#-------------------------------------------------
# messtest objects
#-------------------------------------------------

MESSTEST_OBJS =	\
	$(MESSOBJ)/pile.o						\
	$(MESSOBJ)/tools/messtest/main.o		\
	$(MESSOBJ)/tools/messtest/core.o		\
	$(MESSOBJ)/tools/messtest/testmess.o	\
	$(MESSOBJ)/tools/messtest/testimgt.o	\
	$(MESSOBJ)/tools/messtest/testzpth.o	\
	$(MESSOBJ)/tools/messtest/tststubs.o	\
	$(MESSOBJ)/tools/messtest/tstutils.o	\



#-------------------------------------------------
# rules to build the messtest executable
#-------------------------------------------------

#$(MESSTEST_OBJS): maketree

$(MESSTEST): $(VERSIONOBJ) $(MESSTEST_OBJS) $(LIBIMGTOOL) $(DRVLIBS) $(LIBEMU) $(LIBCPU) $(LIBDASM) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@
