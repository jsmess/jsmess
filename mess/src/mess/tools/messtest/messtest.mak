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
MESSTESTOBJ = $(MESS_TOOLS)/messtest



#-------------------------------------------------
# messtest objects
#-------------------------------------------------

# messtest directories
OBJDIRS += $(MESSTESTOBJ)

MESSTEST_OBJS =	\
	$(MESS_TOOLS)/pile.o		\
	$(MESSTESTOBJ)/main.o		\
	$(MESSTESTOBJ)/core.o		\
	$(MESSTESTOBJ)/testmess.o	\
	$(MESSTESTOBJ)/testimgt.o	\
	$(MESSTESTOBJ)/testzpth.o	\
	$(MESSTESTOBJ)/tststubs.o	\
	$(MESSTESTOBJ)/tstutils.o	\



#-------------------------------------------------
# rules to build the messtest executable
#-------------------------------------------------

#$(MESSTEST_OBJS): maketree

$(MESSTEST): $(VERSIONOBJ) $(MESSTEST_OBJS) $(LIBIMGTOOL) $(DRVLIBS) $(LIBCPU) $(LIBEMU) $(LIBDASM) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(SOFTFLOAT) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@
