#pragma once

#ifndef __ABC830__
#define __ABC830__


#include "emu.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/lux10828.h"
#include "machine/lux21046.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ABC830_ADD() \
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(abc830_floppy_interface)

#define MCFG_ABC832_ADD() \
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(abc832_floppy_interface)

#define MCFG_ABC834_ADD() \
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(abc832_floppy_interface)

#define MCFG_ABC838_ADD() \
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(abc838_floppy_interface)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// floppy configuration
extern const floppy_interface abc830_floppy_interface;
extern const floppy_interface abc832_floppy_interface;
extern const floppy_interface abc834_floppy_interface;
extern const floppy_interface abc838_floppy_interface;
extern const floppy_interface fd2_floppy_interface;


// conkort interfaces
extern const input_device_default DEVICE_INPUT_DEFAULTS_NAME( abc830_slow )[];
extern const input_device_default DEVICE_INPUT_DEFAULTS_NAME( abc830_fast )[];
extern const input_device_default DEVICE_INPUT_DEFAULTS_NAME( abc832_fast )[];
extern const input_device_default DEVICE_INPUT_DEFAULTS_NAME( abc834_fast )[];
extern const input_device_default DEVICE_INPUT_DEFAULTS_NAME( abc838_fast )[];


#endif
