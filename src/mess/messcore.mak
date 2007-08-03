###########################################################################
#
#   messcore.mak
#
#   MESS emulation core makefile
#
###########################################################################


#-------------------------------------------------
# MESS core defines
#-------------------------------------------------

COREDEFS += -DMESS



#-------------------------------------------------
# MESS core objects
#-------------------------------------------------

MESS_EMUOBJ = $(OBJ)/mess

EMUOBJS += \
	$(MESS_EMUOBJ)/mess.o		\
	$(MESS_EMUOBJ)/messopts.o	\
	$(MESS_EMUOBJ)/configms.o	\
	$(MESS_EMUOBJ)/mesvalid.o	\
	$(MESS_EMUOBJ)/image.o		\
	$(MESS_EMUOBJ)/device.o		\
	$(MESS_EMUOBJ)/hashfile.o	\
	$(MESS_EMUOBJ)/inputx.o		\
	$(MESS_EMUOBJ)/artworkx.o	\
	$(MESS_EMUOBJ)/uimess.o		\
	$(MESS_EMUOBJ)/filemngr.o	\
	$(MESS_EMUOBJ)/tapectrl.o	\
	$(MESS_EMUOBJ)/compcfg.o	\
	$(MESS_EMUOBJ)/utils.o		\
	$(MESS_EMUOBJ)/eventlst.o	\
	$(MESS_EMUOBJ)/mscommon.o	\
	$(MESS_EMUOBJ)/tagpool.o	\
	$(MESS_EMUOBJ)/cheatms.o	\
	$(MESS_EMUOBJ)/opresolv.o	\
	$(MESS_EMUOBJ)/muitext.o	\
	$(MESS_EMUOBJ)/infomess.o	\
	$(MESS_EMUOBJ)/climess.o	\

$(LIBEMU): $(EMUOBJS)
