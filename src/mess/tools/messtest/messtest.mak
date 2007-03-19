#-------------------------------------------------
# messtest
#-------------------------------------------------

OBJDIRS += $(OBJ)/mess/tools/messtest

MESSTEST_OBJS =								\
	$(EXPAT)								\
	$(IMGTOOL_LIB_OBJS)						\
	$(OBJ)/mess/pile.o						\
	$(OBJ)/mess/tools/messtest/main.o		\
	$(OBJ)/mess/tools/messtest/core.o		\
	$(OBJ)/mess/tools/messtest/testmess.o	\
	$(OBJ)/mess/tools/messtest/testimgt.o	\
	$(OBJ)/mess/tools/messtest/tststubs.o	\
	$(OBJ)/mess/tools/messtest/tstutils.o	\

messtest$(EXE):	$(MESSTEST_OBJS) $(DRVLIBS) $(LIBEMU)  $(LIBCPU) $(LIBSOUND) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(EXPAT) -o $@
