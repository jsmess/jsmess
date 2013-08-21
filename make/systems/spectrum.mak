################################################################################
# Contains Makefile logic required when building ZX Spectrum
################################################################################

# Spectrum has a bios that is required to run the system.
BIOS := spectrum.zip
# SUBTARGET for the MESS makefile.
SUBTARGET := spectrum
MESS_ARGS := ["spectrum","-verbose","-rompath",".","-window","-resolution","352x296","-dump",gamename,"-nokeepaspect"]

# System-specific flags that should be passed to MESS's makefile.
# MESS_FLAGS +=
