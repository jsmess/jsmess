#-------------------------------------------------
# messdocs
#-------------------------------------------------

OBJDIRS += $(OBJ)/mess/tools/messdocs

MESSDOCS_OBJS =								\
	$(OBJ)/mess/tools/messdocs/messdocs.o	\
	$(OBJ)/mess/utils.o						\
	$(OBJ)/mess/toolerr.o					\

messdocs$(EXE):	$(MESSDOCS_OBJS) $(LIBUTIL) $(LIBOCORE) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@
