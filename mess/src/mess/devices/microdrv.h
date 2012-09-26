/*********************************************************************

    microdrv.h

    MESS interface to the Sinclair Microdrive image abstraction code

*********************************************************************/

#pragma once

#ifndef __MICRODRV__
#define __MICRODRV__

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
typedef struct microdrive_config_t	microdrive_config;
struct microdrive_config_t
{
	devcb_write_line	out_comms_out_func;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

WRITE_LINE_DEVICE_HANDLER( microdrive_clk_w );
WRITE_LINE_DEVICE_HANDLER( microdrive_comms_in_w );
WRITE_LINE_DEVICE_HANDLER( microdrive_erase_w );
WRITE_LINE_DEVICE_HANDLER( microdrive_read_write_w );
WRITE_LINE_DEVICE_HANDLER( microdrive_data1_w );
WRITE_LINE_DEVICE_HANDLER( microdrive_data2_w );
READ_LINE_DEVICE_HANDLER( microdrive_data1_r );
READ_LINE_DEVICE_HANDLER( microdrive_data2_r );

DECLARE_LEGACY_IMAGE_DEVICE(MICRODRIVE, microdrive);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDV_1 "mdv1"
#define MDV_2 "mdv2"

#define MCFG_MICRODRIVE_ADD(_tag, _config) \
	MCFG_DEVICE_ADD(_tag, MICRODRIVE, 0) \
	MCFG_DEVICE_CONFIG(_config)

#define MICRODRIVE_CONFIG(_name) \
	const microdrive_config (_name) =

#endif
