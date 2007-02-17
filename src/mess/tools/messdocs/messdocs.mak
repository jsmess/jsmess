#-------------------------------------------------
# messdocs
#-------------------------------------------------

OBJDIRS += $(OBJ)/mess/tools/messdocs

MESSDOCS_OBJS =								\
	$(OBJ)/emu/mamecore.o					\
	$(OBJ)/mess/tools/messdocs/messdocs.o	\
	$(OBJ)/mess/utils.o						\
	$(OBJ)/mess/pool.o						\
	$(OBJ)/mess/toolerr.o					\

messdocs$(EXE):	$(MESSDOCS_OBJS) $(LIBOCORE) $(EXPAT)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@
