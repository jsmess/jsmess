###########################################################################
#
#   castool.mak
#
#   MESS castool makefile
#
###########################################################################


# castool executable name
CASTOOL = castool$(EXE)

# add path to castool headers
CFLAGS += -I$(SRC)/$(TARGET)/tools/castool

# castool directories
CASTOOLOBJ = $(MESS_TOOLS)/castool



#-------------------------------------------------
# castool objects
#-------------------------------------------------

OBJDIRS += \
	$(CASTOOLOBJ)

LIBCASTOOL = $(OBJ)/libcastool.a

# castool lib objects
CASTOOL_LIB_OBJS =						\
	$(OBJ)/version.o					\
	$(EMUOBJ)/emualloc.o				\
	$(EMUOBJ)/emucore.o					\
	$(EMUOBJ)/emuopts.o 				\
	$(EMUOBJ)/memory.o					\


$(LIBCASTOOL): $(CASTOOL_LIB_OBJS)

CASTOOL_OBJS = \
	$(CASTOOLOBJ)/main.o \
	$(CASTOOLOBJ)/stubs.o \
	$(MESS_TOOLS)/toolerr.o



#-------------------------------------------------
# rules to build the castool executable
#-------------------------------------------------

$(CASTOOL): $(CASTOOL_OBJS) $(LIBCASTOOL) $(LIBUTIL) $(EXPAT) $(FORMATS_LIB) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
