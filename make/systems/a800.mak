################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := a800.zip a400.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := a800
MESS_ARGS := ["a800","-verbose","-rompath",".","-cart1",gamename,"-window","-resolution","336x225","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
