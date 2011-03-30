/**********************************************************************

    Commodore 9060/9090 Hard Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - everything

*/

#include "emu.h"
#include "c9060.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_TAG		"7e"
#define M6532_0_TAG		"7f"
#define M6532_1_TAG		"7g"

#define M6504_TAG		"4a"
#define M6522_TAG		"4b"

#define C9060_REGION	"c9060"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c9060_t c9060_t;
struct _c9060_t
{
	/* IEEE-488 bus */
	int address;						/* device address - 8 */
	int rfdo;							/* not ready for data output */
	int daco;							/* not data accepted output */
	int atna;							/* attention acknowledge */

	/* devices */
	device_t *cpu_dos;
	device_t *cpu_hdc;
	device_t *riot0;
	device_t *riot1;
	device_t *via;
	device_t *bus;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c9060_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C9060) || (device->type() == C9090));
	return (c9060_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c9060_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C9060) || (device->type() == C9090));
	return (c9060_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE void update_ieee_signals(device_t *device)
{
	c9060_t *c9060 = get_safe_token(device);

	int atn = ieee488_atn_r(c9060->bus);
	int atna = c9060->atna;
	int rfdo = c9060->rfdo;
	int daco = c9060->daco;

	int nrfd = !(!(!(atn&atna)&rfdo)|!(atn|atna));
	int ndac = !(daco|!(atn|atna));

	ieee488_nrfd_w(c9060->bus, device, nrfd);
	ieee488_ndac_w(c9060->bus, device, ndac);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    c9060_ieee488_atn_w - IEEE-488 bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c9060_ieee488_atn_w )
{
	c9060_t *c9060 = get_safe_token(device);

	update_ieee_signals(device);

	/* set RIOT PA7 */
	riot6532_porta_in_set(c9060->riot1, !state << 7, 0x80);
}

/*-------------------------------------------------
    c9060_ieee488_ifc_w - IEEE-488 bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c9060_ieee488_ifc_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c9060_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c9060_dos_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c9060:c9060", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c9060_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c9060_fdc_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE_MODERN(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1800, 0x1fff) AM_ROM AM_REGION("c9060:c9060", 0x2000) // 6530
ADDRESS_MAP_END

/*-------------------------------------------------
    riot6532_interface riot0_intf 7f
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( dio_r )
{
	/*

        bit     description

        PA0     DI0
        PA1     DI1
        PA2     DI2
        PA3     DI3
        PA4     DI4
        PA5     DI5
        PA6     DI6
        PA7     DI7

    */

	c9060_t *c9060 = get_safe_token(device->owner());

	return ieee488_dio_r(c9060->bus, 0);
}

static WRITE8_DEVICE_HANDLER( dio_w )
{
	/*

        bit     description

        PB0     DO0
        PB1     DO1
        PB2     DO2
        PB3     DO3
        PB4     DO4
        PB5     DO5
        PB6     DO6
        PB7     DO7

    */

	c9060_t *c9060 = get_safe_token(device->owner());

	ieee488_dio_w(c9060->bus, device->owner(), data);
}

static const riot6532_interface riot0_intf =
{
	DEVCB_HANDLER(dio_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(dio_w),
	DEVCB_NULL
};

/*-------------------------------------------------
    riot6532_interface riot1_intf ue1
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( riot1_pa_r )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	c9060_t *c9060 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* end or identify in */
	data |= ieee488_eoi_r(c9060->bus) << 5;

	/* data valid in */
	data |= ieee488_dav_r(c9060->bus) << 6;

	/* attention */
	data |= !ieee488_atn_r(c9060->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( riot1_pa_w )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	c9060_t *c9060 = get_safe_token(device->owner());

	/* attention acknowledge */
	c9060->atna = BIT(data, 0);

	/* data accepted out */
	c9060->daco = BIT(data, 1);

	/* not ready for data out */
	c9060->rfdo = BIT(data, 2);

	/* end or identify out */
	ieee488_eoi_w(c9060->bus, device->owner(), BIT(data, 3));

	/* data valid out */
	ieee488_dav_w(c9060->bus, device->owner(), BIT(data, 4));

	update_ieee_signals(device->owner());
}

static READ8_DEVICE_HANDLER( riot1_pb_r )
{
	/*

        bit     description

        PB0
        PB1
        PB2
        PB3
        PB4     DRIVE RDY
        PB5     PWR ON AND NO ERRORS
        PB6     DACI
        PB7     RFDI

    */

	c9060_t *c9060 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* data accepted in */
	data |= ieee488_ndac_r(c9060->bus) << 6;

	/* ready for data in */
	data |= ieee488_nrfd_r(c9060->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( riot1_pb_w )
{
	/*

        bit     description

        PB0
        PB1
        PB2
        PB3
        PB4     DRIVE RDY
        PB5     PWR ON AND NO ERRORS
        PB6     DACI
        PB7     RFDI

    */
}

static WRITE_LINE_DEVICE_HANDLER( riot1_irq_w )
{
	c9060_t *c9060 = get_safe_token(device->owner());

	device_set_input_line(c9060->cpu_dos, M6502_IRQ_LINE, state);
}

static const riot6532_interface riot1_intf =
{
	DEVCB_HANDLER(riot1_pa_r),
	DEVCB_HANDLER(riot1_pb_r),
	DEVCB_HANDLER(riot1_pa_w),
	DEVCB_HANDLER(riot1_pb_w),
	DEVCB_LINE(riot1_irq_w)
};

/*-------------------------------------------------
    via6522_interface via_intf 4b
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( via_pa_r )
{
	/*

        bit     description

        PA0     DB0
        PA1     DB1
        PA2     DB2
        PA3     DB3
        PA4     DB4
        PA5     DB5
        PA6     DB6
        PA7     DB7

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via_pa_w )
{
	/*

        bit     description

        PA0     DB0
        PA1     DB1
        PA2     DB2
        PA3     DB3
        PA4     DB4
        PA5     DB5
        PA6     DB6
        PA7     DB7

    */
}

static READ8_DEVICE_HANDLER( via_pb_r )
{
	/*

        bit     description

        PB0     SEL
        PB1     RST
        PB2     C/D
        PB3     BUSY
        PB4     J14
        PB5     J14
        PB6     I/O
        PB7     MSG

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via_pb_w )
{
	/*

        bit     description

        PB0     SEL
        PB1     RST
        PB2     C/D
        PB3     BUSY
        PB4     J14
        PB5     J14
        PB6     I/O
        PB7     MSG

    */
}

static const via6522_interface via_intf =
{
	DEVCB_HANDLER(via_pa_r),
	DEVCB_HANDLER(via_pb_r),
	DEVCB_NULL, // ACK
	DEVCB_NULL,
	DEVCB_NULL, // MSG
	DEVCB_NULL, // ?

	DEVCB_HANDLER(via_pa_w),
	DEVCB_HANDLER(via_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c9060 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c9060 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c9060_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c9060_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, via_intf)

	// Tandon TM602S
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c9090 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c9090 )
	MCFG_FRAGMENT_ADD(c9060)

	// Tandon TM603S
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c9060 )
-------------------------------------------------*/

ROM_START( c9060 ) // schematic 300010
	ROM_REGION( 0x4800, C9060_REGION, ROMREGION_LOADBYNAME )
	ROM_LOAD_OPTIONAL( "300516-revb.7c", 0x0000, 0x2000, CRC(2d758a14) SHA1(c959cc9dde84fc3d64e95e58a0a096a26d8107fd) )
	ROM_LOAD( "300516-revc.7c", 0x0000, 0x2000, CRC(d6a3e88f) SHA1(bb1ddb5da94a86266012eca54818aa21dc4cef6a) )
	ROM_LOAD_OPTIONAL( "300517-reva.7d", 0x2000, 0x2000, CRC(566df630) SHA1(b1602dfff408b165ee52a6a4ca3e2ec27e689ba9) )
	ROM_LOAD_OPTIONAL( "300517-revb.7d", 0x2000, 0x2000, CRC(f0382bc3) SHA1(0b0a8dc520f5b41ffa832e4a636b3d226ccbb7f1) )
	ROM_LOAD( "300517-revc.7d", 0x2000, 0x2000, CRC(2a9ad4ad) SHA1(4c17d014de48c906871b9b6c7d037d8736b1fd52) )

	ROM_LOAD_OPTIONAL( "300515-reva.4c", 0x4000, 0x0800, CRC(99e096f7) SHA1(a3d1deb27bf5918b62b89c27fa3e488eb8f717a4) )
	ROM_LOAD( "300515-revb.4c", 0x4000, 0x0800, CRC(49adf4fb) SHA1(59dafbd4855083074ba8dc96a04d4daa5b76e0d6) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c9060 )
-------------------------------------------------*/

static DEVICE_START( c9060 )
{
	c9060_t *c9060 = get_safe_token(device);
	const c9060_config *config = get_safe_config(device);

	/* find our CPU */
	c9060->cpu_dos = device->subdevice(M6502_TAG);
	c9060->cpu_hdc = device->subdevice(M6504_TAG);

	/* find devices */
	c9060->riot0 = device->subdevice(M6532_0_TAG);
	c9060->riot1 = device->subdevice(M6532_1_TAG);
	c9060->via = device->subdevice(M6522_TAG);
	c9060->bus = device->machine().device(config->bus_tag);

	/* register for state saving */
//  device->save_item(NAME(c9060->));
}

/*-------------------------------------------------
    DEVICE_RESET( c9060 )
-------------------------------------------------*/

static DEVICE_RESET( c9060 )
{
	c9060_t *c9060 = get_safe_token(device);

	/* reset devices */
	c9060->cpu_dos->reset();
	c9060->cpu_hdc->reset();
	c9060->riot0->reset();
	c9060->riot1->reset();
	c9060->via->reset();

	/* toggle M6502 SO */
	device_set_input_line(c9060->cpu_dos, M6502_SET_OVERFLOW, ASSERT_LINE);
	device_set_input_line(c9060->cpu_dos, M6502_SET_OVERFLOW, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c9060 )
-------------------------------------------------*/

DEVICE_GET_INFO( c9060 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c9060_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c9060_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c9060);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c9060);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c9060);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c9060);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 9060");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore PET");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c9090 )
-------------------------------------------------*/

DEVICE_GET_INFO( c9090 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c9090);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 9090");							break;

		default:										DEVICE_GET_INFO_CALL(c9060);								break;
	}
}

DEFINE_LEGACY_DEVICE(C9060, c9060);
DEFINE_LEGACY_DEVICE(C9090, c9090);
