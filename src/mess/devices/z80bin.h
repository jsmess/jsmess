/*********************************************************************

	z80bin.h

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

*********************************************************************/

#ifndef Z80BIN_H
#define Z80BIN_H

#include "device.h"
#include "image.h"


void z80bin_quickload_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

UINT64 z80bin_load_file( mess_image *image, const char *file_type );


#endif /* Z80BIN_H */
