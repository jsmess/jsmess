################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := odyssey2.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := odyssey2
MESS_ARGS := ["odyssey2","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","160x243","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
