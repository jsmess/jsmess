################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := pet2001.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := pet2001
MESS_ARGS := ["pet2001","-verbose","-rompath",".","-window","-resolution","320x200","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
