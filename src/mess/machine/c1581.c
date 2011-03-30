/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - power/activity LEDs

    http://www.unusedino.de/ec64/technical/aay/c1581/ro81main.htm

*/

#include "emu.h"
#include "c1581.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d81_dsk.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6502_TAG		"u1"
#define M8520_TAG		"u5"
#define WD1770_TAG		"u4"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1581_t c1581_t;
struct _c1581_t
{
	/* IEC bus */
	int address;							/* device number */
	int data_out;							/* serial data out */
	int atn_ack;							/* attention acknowledge */
	int ser_dir;							/* fast serial direction */
	int sp_out;								/* fast serial data out */
	int cnt_out;							/* fast serial clock out */

	/* devices */
	device_t *cpu;
	device_t *cia;
	device_t *wd1770;
	device_t *serial_bus;
	device_t *image;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1581_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1581) || (device->type() == C1563));
	return (c1581_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c1581_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1581) || (device->type() == C1563));
	return (c1581_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE void set_iec_data(device_t *device)
{
	c1581_t *c1581 = get_safe_token(device);

	int atn = cbm_iec_atn_r(c1581->serial_bus);
	int data = !c1581->data_out & !(c1581->atn_ack & !atn);

	/* fast serial data */
	if (c1581->ser_dir) data &= c1581->sp_out;

	cbm_iec_data_w(c1581->serial_bus, device, data);
}

INLINE void set_iec_srq(device_t *device)
{
	c1581_t *c1581 = get_safe_token(device);

	int srq = 1;

	/* fast serial clock */
	if (c1581->ser_dir) srq &= c1581->cnt_out;

	cbm_iec_srq_w(c1581->serial_bus, device, srq);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    c1581_iec_atn_w - serial bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_atn_w )
{
	c1581_t *c1581 = get_safe_token(device);

	mos6526_flag_w(c1581->cia, state);
	set_iec_data(device);
}

/*-------------------------------------------------
    c1581_iec_srq_w - serial bus fast clock
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_srq_w )
{
	c1581_t *c1581 = get_safe_token(device);

	if (!c1581->ser_dir)
	{
		mos6526_cnt_w(c1581->cia, state);
	}
}

/*-------------------------------------------------
    c1581_iec_data_w - serial bus fast data
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_data_w )
{
	c1581_t *c1581 = get_safe_token(device);

	if (!c1581->ser_dir)
	{
		mos6526_sp_w(c1581->cia, state);
	}
}

/*-------------------------------------------------
    c1581_iec_reset_w - serial bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_reset_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1581_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1581_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE(M8520_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1581:c1581", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1563_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1563_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE(M8520_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1563:c1563", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    mos6526_interface cia_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( cia_irq_w )
{
	c1581_t *c1581 = get_safe_token(device->owner());

	device_set_input_line(c1581->cpu, M6502_IRQ_LINE, state);
}

static WRITE_LINE_DEVICE_HANDLER( cia_cnt_w )
{
	c1581_t *c1581 = get_safe_token(device->owner());

	/* fast serial clock out */
	c1581->cnt_out = state;
	set_iec_srq(device->owner());
}

static WRITE_LINE_DEVICE_HANDLER( cia_sp_w )
{
	c1581_t *c1581 = get_safe_token(device->owner());

	/* fast serial data out */
	c1581->sp_out = state;
	set_iec_data(device->owner());
}

static READ8_DEVICE_HANDLER( cia_pa_r )
{
	/*

        bit     description

        PA0     SIDE0
        PA1     /RDY
        PA2     /MOTOR
        PA3     DEV# SEL
        PA4     DEV# SEL
        PA5     POWER LED
        PA6     ACT LED
        PA7     /DISK CHNG

    */

	c1581_t *c1581 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* ready */
	data |= !(floppy_drive_get_flag_state(c1581->image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 1;

	/* device number */
	data |= c1581->address << 3;

	/* disk change */
	data |= floppy_dskchg_r(c1581->image) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( cia_pa_w )
{
	/*

        bit     description

        PA0     SIDE0
        PA1     /RDY
        PA2     /MOTOR
        PA3     DEV# SEL
        PA4     DEV# SEL
        PA5     POWER LED
        PA6     ACT LED
        PA7     /DISK CHNG

    */

	c1581_t *c1581 = get_safe_token(device->owner());

	/* side 0 */
	wd17xx_set_side(c1581->wd1770, !BIT(data, 0));

	/* motor */
	int motor = BIT(data, 2);
	floppy_mon_w(c1581->image, motor);
	floppy_drive_set_ready_state(c1581->image, !motor, 1);

	/* TODO power led */

	/* TODO activity led */
}

static READ8_DEVICE_HANDLER( cia_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6     /WPRT
        PB7     ATN IN

    */

	c1581_t *c1581 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* data in */
	data = !cbm_iec_data_r(c1581->serial_bus);

	/* clock in */
	data |= !cbm_iec_clk_r(c1581->serial_bus) << 2;

	/* write protect */
	data |= !floppy_wpt_r(c1581->image) << 6;

	/* attention in */
	data |= !cbm_iec_atn_r(c1581->serial_bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( cia_pb_w )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6     /WPRT
        PB7     ATN IN

    */

	c1581_t *c1581 = get_safe_token(device->owner());

	/* data out */
	c1581->data_out = BIT(data, 1);

	/* clock out */
	cbm_iec_clk_w(c1581->serial_bus, device->owner(), !BIT(data, 3));

	/* attention acknowledge */
	c1581->atn_ack = BIT(data, 4);

	/* fast serial direction */
	c1581->ser_dir = BIT(data, 5);

	set_iec_data(device->owner());
	set_iec_srq(device->owner());
}

static MOS8520_INTERFACE( cia_intf )
{
	XTAL_16MHz/8,
//  DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE(cia_irq_w),
	DEVCB_NULL,
	DEVCB_LINE(cia_cnt_w),
	DEVCB_LINE(cia_sp_w),
	DEVCB_HANDLER(cia_pa_r),
	DEVCB_HANDLER(cia_pa_w),
	DEVCB_HANDLER(cia_pb_r),
	DEVCB_HANDLER(cia_pb_w)
};

/*-------------------------------------------------
    wd17xx_interface wd1770_intf
-------------------------------------------------*/

static const wd17xx_interface wd1770_intf =
{
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1581 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1581 )
	FLOPPY_OPTION( c1581, "d81", "Commodore 1581 Disk Image", d81_dsk_identify, d81_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1581_floppy_config
-------------------------------------------------*/

static const floppy_config c1581_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(c1581),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1581 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1581 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1581_map)

	MCFG_MOS8520_ADD(M8520_TAG, XTAL_16MHz/8, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ wd1770_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1563 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1563 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1563_map)

	MCFG_MOS8520_ADD(M8520_TAG, XTAL_16MHz/8, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ wd1770_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c1581 )
-------------------------------------------------*/

ROM_START( c1581 )
	ROM_REGION( 0x10000, "c1581", ROMREGION_LOADBYNAME )
	ROM_LOAD_OPTIONAL( "beta.u2",	  0x0000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594) )
	ROM_LOAD_OPTIONAL( "318045-01.u2", 0x0000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164) )
	ROM_LOAD( "318045-02.u2", 0x0000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393) )
	ROM_LOAD_OPTIONAL( "jiffydos 1581.u2", 0x8000, 0x8000, CRC(98873d0f) SHA1(65bbf2be7bcd5bdcbff609d6c66471ffb9d04bfe) )
ROM_END

/*-------------------------------------------------
    ROM( c1563 )
-------------------------------------------------*/

ROM_START( c1563 )
	ROM_REGION( 0x8000, "c1563", ROMREGION_LOADBYNAME )
	ROM_LOAD( "1563-rom.bin", 0x0000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1581 )
-------------------------------------------------*/

static DEVICE_START( c1581 )
{
	c1581_t *c1581 = get_safe_token(device);
	const c1581_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 12));
	c1581->address = config->address - 8;

	/* find our CPU */
	c1581->cpu = device->subdevice(M6502_TAG);

	/* find devices */
	c1581->cia = device->subdevice(M8520_TAG);
	c1581->wd1770 = device->subdevice(WD1770_TAG);
	c1581->serial_bus = device->machine().device(config->serial_bus_tag);
	c1581->image = device->subdevice(FLOPPY_0);

	/* register for state saving */
	device->save_item(NAME(c1581->address));
	device->save_item(NAME(c1581->data_out));
	device->save_item(NAME(c1581->atn_ack));
	device->save_item(NAME(c1581->ser_dir));
	device->save_item(NAME(c1581->sp_out));
	device->save_item(NAME(c1581->cnt_out));
}

/*-------------------------------------------------
    DEVICE_RESET( c1581 )
-------------------------------------------------*/

static DEVICE_RESET( c1581 )
{
	c1581_t *c1581 = get_safe_token(device);

	c1581->cpu->reset();
	c1581->cia->reset();
	c1581->wd1770->reset();

	c1581->sp_out = 1;
	c1581->cnt_out = 1;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1581 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1581 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1581_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1581_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1581);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1581);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1581);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1581);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1581");									break;		
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1563 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1563 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1563);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1563);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1563");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1563");									break;				

		default:										DEVICE_GET_INFO_CALL(c1581);								break;
	}
}

DEFINE_LEGACY_DEVICE(C1581, c1581);
DEFINE_LEGACY_DEVICE(C1563, c1563);
