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
	$(EMUOBJ)/ioprocs.o					\
	$(MESS_FORMATS)/a26_cas.o           \
	$(MESS_FORMATS)/apf_apt.o           \
	$(MESS_FORMATS)/cbm_tap.o           \
	$(MESS_FORMATS)/cgen_cas.o          \
	$(MESS_FORMATS)/coco_cas.o          \
	$(MESS_FORMATS)/csw_cas.o           \
	$(MESS_FORMATS)/fmsx_cas.o          \
	$(MESS_FORMATS)/gtp_cas.o           \
	$(MESS_FORMATS)/ace_tap.o          \
	$(MESS_FORMATS)/hect_tap.o          \
	$(MESS_FORMATS)/kim1_cas.o          \
	$(MESS_FORMATS)/lviv_lvt.o          \
	$(MESS_FORMATS)/mz_cas.o            \
	$(MESS_FORMATS)/orao_cas.o          \
	$(MESS_FORMATS)/oric_tap.o          \
	$(MESS_FORMATS)/pmd_pmd.o           \
	$(MESS_FORMATS)/primoptp.o          \
	$(MESS_FORMATS)/rk_cas.o            \
	$(MESS_FORMATS)/sord_cas.o          \
	$(MESS_FORMATS)/svi_cas.o           \
	$(MESS_FORMATS)/trs_cas.o           \
	$(MESS_FORMATS)/tzx_cas.o           \
	$(MESS_FORMATS)/uef_cas.o           \
	$(MESS_FORMATS)/vg5k_cas.o          \
	$(MESS_FORMATS)/vt_cas.o            \
	$(MESS_FORMATS)/zx81_p.o		    \
	$(EMUOBJ)/imagedev/cassimg.o			\
	$(EMUOBJ)/imagedev/wavfile.o			\


$(LIBCASTOOL): $(CASTOOL_LIB_OBJS)

CASTOOL_OBJS = \
	$(CASTOOLOBJ)/main.o \
	$(CASTOOLOBJ)/stubs.o \
	$(MESS_TOOLS)/toolerr.o



#-------------------------------------------------
# rules to build the castool executable
#-------------------------------------------------

$(CASTOOL): $(CASTOOL_OBJS) $(LIBCASTOOL) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
