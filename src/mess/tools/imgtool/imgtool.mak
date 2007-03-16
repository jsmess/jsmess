IMGTOOL_LIB_OBJS =							\
	$(OBJ)/emu/options.o					\
	$(OBJ)/emu/restrack.o					\
	$(OBJ)/emu/mamecore.o					\
	$(OBJ)/version.o						\
	$(OBJ)/mess/tagpool.o					\
	$(OBJ)/mess/utils.o						\
	$(OBJ)/mess/tagpool.o					\
	$(OBJ)/mess/opresolv.o					\
	$(OBJ)/mess/formats/ioprocs.o			\
	$(OBJ)/mess/formats/flopimg.o			\
	$(OBJ)/mess/formats/cassimg.o			\
	$(OBJ)/mess/formats/basicdsk.o			\
	$(OBJ)/mess/formats/coco_dsk.o			\
	$(OBJ)/mess/formats/coco_cas.o			\
	$(OBJ)/mess/formats/pc_dsk.o			\
	$(OBJ)/mess/formats/ap2_dsk.o			\
	$(OBJ)/mess/formats/ap_dsk35.o			\
	$(OBJ)/mess/formats/wavfile.o			\
	$(OBJ)/mess/formats/vt_dsk.o			\
	$(OBJ)/mess/tools/imgtool/stream.o		\
	$(OBJ)/mess/tools/imgtool/library.o		\
	$(OBJ)/mess/tools/imgtool/modules.o		\
	$(OBJ)/mess/tools/imgtool/iflopimg.o	\
	$(OBJ)/mess/tools/imgtool/filter.o		\
	$(OBJ)/mess/tools/imgtool/filteoln.o	\
	$(OBJ)/mess/tools/imgtool/filtbas.o		\
	$(OBJ)/mess/tools/imgtool/macbin.o		\
	$(OBJ)/mess/tools/imgtool/imgtool.o		\
	$(OBJ)/mess/tools/imgtool/imgterrs.o	\
	$(OBJ)/mess/tools/imgtool/rsdos.o		\
	$(OBJ)/mess/tools/imgtool/os9.o			\
	$(OBJ)/mess/tools/imgtool/imghd.o		\
	$(OBJ)/mess/tools/imgtool/mac.o			\
	$(OBJ)/mess/tools/imgtool/ti99.o		\
	$(OBJ)/mess/tools/imgtool/ti990hd.o		\
	$(OBJ)/mess/tools/imgtool/concept.o		\
	$(OBJ)/mess/tools/imgtool/fat.o			\
	$(OBJ)/mess/tools/imgtool/pc_flop.o		\
	$(OBJ)/mess/tools/imgtool/pc_hard.o		\
	$(OBJ)/mess/tools/imgtool/prodos.o		\
	$(OBJ)/mess/tools/imgtool/vzdos.o		\
	$(OBJ)/mess/tools/imgtool/thomson.o		\
	$(OBJ)/mess/tools/imgtool/macutil.o		\
	$(OBJ)/mess/tools/imgtool/cybiko.o
#	  $(OBJ)/mess/tools/imgtool/tstsuite.o \
#	  $(OBJ)/mess/formats/fmsx_cas.o       \
#	  $(OBJ)/mess/formats/svi_cas.o        \
#	  $(OBJ)/mess/formats/ti85_ser.o       \
#	  $(OBJ)/mess/tools/imgtool/imgfile.o  \
#	  $(OBJ)/mess/tools/imgtool/imgwave.o  \
#	  $(OBJ)/mess/tools/imgtool/cococas.o  \
#	  $(OBJ)/mess/tools/imgtool/vmsx_tap.o \
#	  $(OBJ)/mess/tools/imgtool/vmsx_gm2.o \
#	  $(OBJ)/mess/tools/imgtool/fmsx_cas.o \
#	  $(OBJ)/mess/tools/imgtool/svi_cas.o  \
#	  $(OBJ)/mess/tools/imgtool/msx_dsk.o  \
#	  $(OBJ)/mess/tools/imgtool/xsa.o      \
#	  $(OBJ)/mess/tools/imgtool/t64.o      \
#	  $(OBJ)/mess/tools/imgtool/lynx.o     \
#	  $(OBJ)/mess/tools/imgtool/crt.o      \
#	  $(OBJ)/mess/tools/imgtool/d64.o      \
#	  $(OBJ)/mess/tools/imgtool/rom16.o    \
#	  $(OBJ)/mess/tools/imgtool/nccard.o   \
#	  $(OBJ)/mess/tools/imgtool/ti85.o     \



#-------------------------------------------------
# imgtool
#-------------------------------------------------

IMGTOOL_OBJS = \
	$(IMGTOOL_LIB_OBJS) \
	$(OBJ)/mess/tools/imgtool/main.o \
	$(OBJ)/mess/tools/imgtool/stubs.o \
	$(OBJ)/mess/toolerr.o
	
imgtool$(EXE):	 $(IMGTOOL_OBJS) $(LIBUTIL) $(EXPAT) $(ZLIB) $(LIBOCORE)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
