################################################################################
# Contains Makefile logic required when building ColecoVision
################################################################################

# ColecoVision has a bios that is required to run the system.
BIOS := coleco.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := colecovision

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=