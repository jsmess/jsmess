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

int z80bin_load_file(const device_config *image, const char *file_type, UINT16 *exec_addr, UINT16 *start_addr, UINT16 *end_addr );


#endif /* Z80BIN_H */
