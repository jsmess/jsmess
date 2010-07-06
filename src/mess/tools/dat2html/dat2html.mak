###########################################################################
#
#   dat2html.mak
#
#   MESS dat2html makefile
#
###########################################################################


# dat2html executable name
DAT2HTML = dat2html$(EXE)

# dat2html directories
OBJDIRS += \
	$(MESSOBJ)/tools/dat2html \



#-------------------------------------------------
# dat2html objects
#-------------------------------------------------

DAT2HTML_OBJS =								\
	$(EMUOBJ)/emualloc.o				\
	$(EMUOBJ)/emucore.o						\
	$(MESSOBJ)/tools/dat2html/dat2html.o	\
	$(MESSOBJ)/tools/imgtool/stubs.o		\



#-------------------------------------------------
# rules for building the dat2html executable
#-------------------------------------------------

$(DAT2HTML): $(DAT2HTML_OBJS) $(LIBUTIL) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@



#-------------------------------------------------
# rule for building sysinfo.htm
#-------------------------------------------------

sysinfo.htm: $(DAT2HTML)
	@echo Generating $@...
	@$(CURPATH)$(DAT2HTML) sysinfo.dat
