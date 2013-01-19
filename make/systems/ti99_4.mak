################################################################################
# Contains Makefile logic required when building Channel F
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := ti99_4.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := ti99_4
MESS_ARGS := ["ti99_4","-verbose","-rompath",".","-window","-resolution","286x222","-cart1",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
