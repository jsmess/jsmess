################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := apple2c.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := apple2c
MESS_ARGS := ["apple2c","-verbose","-rompath",".","-window","-resolution","560x192","-flop1",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
