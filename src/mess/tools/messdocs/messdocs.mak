###########################################################################
#
#   messdocs.mak
#
#   MESS messdocs makefile
#
###########################################################################


# messdocs executable name
MESSDOCS = messdocs$(EXE)

# messdocs will create this file
HELPOBJ = $(OBJ)/help
MESSHELP = mess.chm

# add our stuff to the global variables
OBJDIRS += $(MESS_TOOLS)/messdocs

# html help compiler
HHC = @-hhc



#-------------------------------------------------
# messdocs objects
#-------------------------------------------------

MESSDOCS_OBJS = \
	$(MESSOBJ)/tools/messdocs/messdocs.o	\
	$(MESS_TOOLS)/toolerr.o					\



#-------------------------------------------------
# rules to build the messdocs executable
#-------------------------------------------------

$(OBJ)/build/$(MESSDOCS): $(MESSDOCS_OBJS) $(LIBEMU) $(LIBUTIL) $(LIBOCORE) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@



#-------------------------------------------------
# rule for building the mess help file
#-------------------------------------------------

help: maketree $(MESSHELP)

$(MESSHELP): $(OBJ)/build/$(MESSDOCS) sysinfo.dat
	$(subst /,\,$(OBJ)/build/$(MESSDOCS)) docs/wintoc.xml $(HELPOBJ)
	$(HHC) $(subst /,\\,$(HELPOBJ))\\mess.hhp
	@cp $(HELPOBJ)/$(MESSHELP) $@
