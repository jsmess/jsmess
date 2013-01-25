################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS :=
# SUBTARGET for the MESS makefile.
SUBTARGET := genesis
# Arguments to MESS when running a game.
MESS_ARGS = ["genesis","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","256x224","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
