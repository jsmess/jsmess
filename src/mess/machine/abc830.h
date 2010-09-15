#ifndef __CONKORT__
#define __CONKORT__

#include "emu.h"
#include "formats/flopimg.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(ABC830_PIO, abc830_pio);
DECLARE_LEGACY_DEVICE(ABC830, abc830);
DECLARE_LEGACY_DEVICE(ABC832, abc832);
DECLARE_LEGACY_DEVICE(ABC838, abc838);

#define ADDRESS_ABC832			44
#define ADDRESS_ABC830			45
#define ADDRESS_ABC838			46

#define DRIVE_TEAC_FD55F		0x01
#define DRIVE_BASF_6138			0x02
#define DRIVE_MICROPOLIS_1015F	0x03
#define DRIVE_BASF_6118			0x04
#define DRIVE_MICROPOLIS_1115F	0x05
#define DRIVE_BASF_6106_08		0x08
#define DRIVE_MPI_51			0x09
#define DRIVE_BASF_6105			0x0e
#define DRIVE_BASF_6106			0x0f

#define MDRV_ABC830_PIO_ADD(_tag, _bus_tag, _drive) \
	MDRV_DEVICE_ADD(_tag, ABC830_PIO, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw1, 0x03) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw2, _drive) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, address, ADDRESS_ABC830)

#define MDRV_ABC830_ADD(_tag, _bus_tag, _drive) \
	MDRV_DEVICE_ADD(_tag, ABC830, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw1, 0x03) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw2, _drive) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, address, ADDRESS_ABC830)

#define MDRV_ABC832_ADD(_tag, _bus_tag, _drive) \
	MDRV_DEVICE_ADD(_tag, ABC832, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw1, 0x03) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw2, _drive) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, address, ADDRESS_ABC832)

#define MDRV_ABC838_ADD(_tag, _bus_tag, _drive) \
	MDRV_DEVICE_ADD(_tag, ABC838, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(conkort_config, bus_tag, _bus_tag) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw1, 0x03) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, sw2, _drive) \
    MDRV_DEVICE_CONFIG_DATA32(conkort_config, address, ADDRESS_ABC838)

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
	UINT8 sw1;					/* single/double sided/density */
	UINT8 sw2;					/* drive type */
	UINT8 address;				/* ABC bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* configuration dips */
INPUT_PORTS_EXTERN( luxor_55_10828 );
INPUT_PORTS_EXTERN( luxor_55_21046 );

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
