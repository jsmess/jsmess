#ifndef __CONKORT__
#define __CONKORT__

#include "emu.h"
#include "formats/flopimg.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define LUXOR_55_10828 DEVICE_GET_INFO_NAME( luxor_55_10828 )
#define LUXOR_55_21046 DEVICE_GET_INFO_NAME( luxor_55_21046 )

#define MDRV_LUXOR_55_10828_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, LUXOR_55_10828, 0)

#define MDRV_LUXOR_55_21046_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, LUXOR_55_21046, 0)

#define LUXOR_55_10828_ABCBUS(_tag) \
	_tag, DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_cs_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_stat_r), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_inp_r), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_utp_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_c1_w), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_c3_w), DEVCB_DEVICE_LINE(_tag, luxor_55_10828_rst_w)

#define LUXOR_55_21046_ABCBUS(_tag) \
	_tag, DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_cs_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_stat_r), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_inp_r), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_utp_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_c1_w), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_c3_w), DEVCB_DEVICE_LINE(_tag, luxor_55_21046_rst_w)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( luxor_55_10828 );
DEVICE_GET_INFO( luxor_55_21046 );

/* configuration dips */
INPUT_PORTS_EXTERN( luxor_55_21046 );

/* floppy options */
FLOPPY_OPTIONS_EXTERN( abc80 );

/* ABC bus interface */
WRITE8_DEVICE_HANDLER( luxor_55_10828_cs_w );
WRITE_LINE_DEVICE_HANDLER( luxor_55_10828_rst_w );
READ8_DEVICE_HANDLER( luxor_55_10828_stat_r );
READ8_DEVICE_HANDLER( luxor_55_10828_inp_r );
WRITE8_DEVICE_HANDLER( luxor_55_10828_utp_w );
WRITE8_DEVICE_HANDLER( luxor_55_10828_c1_w );
WRITE8_DEVICE_HANDLER( luxor_55_10828_c3_w );

WRITE8_DEVICE_HANDLER( luxor_55_21046_cs_w );
WRITE_LINE_DEVICE_HANDLER( luxor_55_21046_rst_w );
READ8_DEVICE_HANDLER( luxor_55_21046_stat_r );
READ8_DEVICE_HANDLER( luxor_55_21046_inp_r );
WRITE8_DEVICE_HANDLER( luxor_55_21046_utp_w );
WRITE8_DEVICE_HANDLER( luxor_55_21046_c1_w );
WRITE8_DEVICE_HANDLER( luxor_55_21046_c3_w );

#endif
