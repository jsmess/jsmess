###########################################################################
#
#   messdocs.mak
#
#   MESS messdocs makefile
#
###########################################################################


# messdocs executable name
MESSDOCS = messdocs$(EXE)

# messdocs directories
OBJDIRS += $(MESSOBJ)/tools/messdocs



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

messdocs$(EXE):	$(MESSDOCS_OBJS) $(LIBUTIL) $(LIBOCORE) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@



#-------------------------------------------------
# rule for building the mess help file
#-------------------------------------------------

mess.chm: $(MESSDOCS)
	$(MESSDOCS) docs/wintoc.xml $(OBJ)/help
	$(HHC) $(subst /,\\,$(OBJ)/help)\\mess.hhp
	@cp $(OBJ)/help/mess.chm $@
