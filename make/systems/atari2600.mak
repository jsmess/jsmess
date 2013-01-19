################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

SUBTARGET := atari2600
MESS_ARGS := ["a2600","-verbose","-rompath",".","-cart",gamename,"-window","-resolution","176x223","-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=