#ifndef __CONKORT__
#define __CONKORT__

#include "emu.h"
#include "formats/flopimg.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(LUXOR_55_10828, luxor_55_10828);
DECLARE_LEGACY_DEVICE(LUXOR_55_21046, luxor_55_21046);

#define MDRV_LUXOR_55_10828_ADD(_tag, _bus_tag) \
	MDRV_DEVICE_ADD(_tag, LUXOR_55_10828, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag)

#define MDRV_LUXOR_55_21046_ADD(_tag, _bus_tag) \
	MDRV_DEVICE_ADD(_tag, LUXOR_55_21046, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag)

#define LUXOR_55_10828_ABCBUS(_tag) \
	_tag, DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_cs_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_stat_r), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_inp_r), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_utp_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_c1_w), \
	DEVCB_NULL, DEVCB_DEVICE_HANDLER(_tag, luxor_55_10828_c3_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, luxor_55_10828_rst_w)

#define LUXOR_55_21046_ABCBUS(_tag) \
	_tag, DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_cs_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_stat_r), \
	DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_inp_r), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_utp_w), DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_c1_w), \
	DEVCB_NULL, DEVCB_DEVICE_HANDLER(_tag, luxor_55_21046_c3_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, luxor_55_21046_rst_w)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _conkort_config conkort_config;
struct _conkort_config
{
	const char *bus_tag;		/* bus device */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
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
