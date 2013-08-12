################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := cat.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := cat
MESS_ARGS := ["cat","-verbose","-rompath",".","-window","-resolution","672x344","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
