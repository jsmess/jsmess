/*********************************************************************

	z80bin.h

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

*********************************************************************/

#ifndef Z80BIN_H
#define Z80BIN_H

void z80bin_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

#endif /* Z80BIN_H */
