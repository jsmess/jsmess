/**********************************************************************

    IEEE-488.1 General Purpose Interface Bus emulation
	(aka HP-IB, GPIB, CBM IEEE)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __IEEE488__
#define __IEEE488__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define IEEE488 DEVICE_GET_INFO_NAME( ieee488 )

#define MDRV_IEEE488_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, IEEE488, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define IEEE488_DAISY(_name) \
	const ieee488_daisy_chain (_name)[] =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ieee488_daisy_chain ieee488_daisy_chain;
struct _ieee488_daisy_chain
{
	const char *tag;	/* device tag */

	devcb_write_line	out_eoi_func;
	devcb_write_line	out_dav_func;
	devcb_write_line	out_nrfd_func;
	devcb_write_line	out_ndac_func;
	devcb_write_line	out_ifc_func;
	devcb_write_line	out_srq_func;
	devcb_write_line	out_atn_func;
	devcb_write_line	out_ren_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( ieee488 );

/* end or identify */
void ieee488_eoi_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_eoi_r );

/* data valid */
void ieee488_dav_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_dav_r );

/* not ready for data */
void ieee488_nrfd_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_nrfd_r );

/* not data accepted */
void ieee488_ndac_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_ndac_r );

/* interface clear */
void ieee488_ifc_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_ifc_r );

/* service request */
void ieee488_srq_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_srq_r );

/* attention */
void ieee488_atn_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_atn_r );

/* remote enable */
void ieee488_ren_w(const device_config *bus, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( ieee488_ren_r );

/* data */
READ8_DEVICE_HANDLER( ieee488_dio_r );
void ieee488_dio_w(const device_config *bus, const device_config *device, UINT8 data);

#endif
