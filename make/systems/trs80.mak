################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := trs80.zip trs80l2.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := trs80
MESS_ARGS := ["trs80","-verbose","-rompath",".","-window","-resolution","384x192","-cass",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
