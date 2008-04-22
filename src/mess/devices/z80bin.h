/*********************************************************************

	z80bin.h

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

*********************************************************************/

#ifndef __Z80BIN_H__
#define __Z80BIN_H__

#include "image.h"


/***************************************************************************
    MACROS
***************************************************************************/

#define Z80BIN	DEVICE_GET_INFO_NAME(z80bin)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* device getinfo function */
DEVICE_GET_INFO(z80bin);

/* please stop using this method - z80bin needs to be encapsulated into a proper device */
int z80bin_load_file(const device_config *image, const char *file_type, UINT16 *exec_addr, UINT16 *start_addr, UINT16 *end_addr );


#endif /* __Z80BIN_H__ */
