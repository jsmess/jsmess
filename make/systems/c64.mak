################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := c64.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := c64
MESS_ARGS := ["c64","-verbose","-rompath",".","-window","-resolution","418x235","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
