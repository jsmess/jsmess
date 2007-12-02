###########################################################################
#
#   messdocs.mak
#
#   MESS messdocs makefile
#
###########################################################################


# messdocs executable name
MESSDOCS = messdocs$(EXE)

MESSHELP = mess.chm

# messdocs directories
OBJDIRS += $(MESSOBJ)/tools/messdocs

HELPOBJ = $(OBJ)/help


#-------------------------------------------------
# messdocs objects
#-------------------------------------------------

MESSDOCS_OBJS = \
	$(MESSOBJ)/tools/messdocs/messdocs.o	\
	$(MESSOBJ)/utils.o						\
	$(MESSOBJ)/toolerr.o					\



#-------------------------------------------------
# rules to build the messdocs executable
#-------------------------------------------------

$(OBJ)/build/$(MESSDOCS): $(MESSDOCS_OBJS) $(LIBUTIL) $(LIBOCORE) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@



#-------------------------------------------------
# rule for building the mess help file
#-------------------------------------------------

$(MESSHELP): $(OBJ)/build/$(MESSDOCS)
	$(subst /,\,$(OBJ)/build/$(MESSDOCS)) docs/wintoc.xml $(HELPOBJ)
	$(HHC) $(subst /,\\,$(HELPOBJ))\\mess.hhp
	@cp $(HELPOBJ)/$(MESSHELP) $@
