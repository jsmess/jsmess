###########################################################################
#
#   emudummy.mak
#
#   Not a system!  Rather, required hardware for all builds.
#
###########################################################################

CPUS += Z80
CPUS += MCS48

DRVLIBS = $(EMUOBJ)/drivers/emudummy.o
