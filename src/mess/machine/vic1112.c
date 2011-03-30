/**********************************************************************

    Commodore VIC-1112 IEEE-488 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "vic1112.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6522_0_TAG		"u4"
#define M6522_1_TAG		"u5"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vic1112_t vic1112_t;
struct _vic1112_t
{
	int via0_irq;						/* VIA #0 interrupt request */
	int via1_irq;						/* VIA #1 interrupt request */

	/* devices */
	via6522_device *via0;
	via6522_device *via1;
	via6522_device *bus;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vic1112_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == VIC1112);
	return (vic1112_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE vic1112_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == VIC1112);
	return (vic1112_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    vic1112_ieee488_srq_w - IEEE-488 service
    request
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vic1112_ieee488_srq_w )
{
	vic1112_t *vic1112 = get_safe_token(device);

	vic1112->bus->write_cb1(state);
}

/*-------------------------------------------------
    via6522_interface vic1112_via0_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via0_irq_w )
{
	vic1112_t *vic1112 = get_safe_token(device->owner());

	vic1112->via0_irq = state;

	device_set_input_line(device->machine().firstcpu, M6502_IRQ_LINE, (vic1112->via0_irq | vic1112->via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */

	vic1112_t *vic1112 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* end or identify */
	data |= ieee488_eoi_r(vic1112->bus) << 3;

	/* data valid in */
	data |= ieee488_dav_r(vic1112->bus) << 4;

	/* not ready for data in */
	data |= ieee488_nrfd_r(vic1112->bus) << 5;

	/* not data accepted in */
	data |= ieee488_ndac_r(vic1112->bus) << 6;

	/* attention in */
	data |= ieee488_atn_r(vic1112->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI IN
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */

	vic1112_t *vic1112 = get_safe_token(device->owner());

	/* data valid out */
	ieee488_dav_w(vic1112->bus, device->owner(), BIT(data, 0));

	/* not ready for data out */
	ieee488_nrfd_w(vic1112->bus, device->owner(), BIT(data, 1));

	/* not data accepted out */
	ieee488_ndac_w(vic1112->bus, device->owner(), BIT(data, 2));
}

static const via6522_interface vic1112_via0_intf =
{
	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface vic1112_via1_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	vic1112_t *vic1112 = get_safe_token(device->owner());

	vic1112->via1_irq = state;

	device_set_input_line(device->machine().firstcpu, M6502_IRQ_LINE, (vic1112->via0_irq | vic1112->via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( dio_r )
{
	/*

        bit     description

        PB0     DI1
        PB1     DI2
        PB2     DI3
        PB3     DI4
        PB4     DI5
        PB5     DI6
        PB6     DI7
        PB7     DI8

    */

	vic1112_t *vic1112 = get_safe_token(device->owner());

	return ieee488_dio_r(vic1112->bus, 0);
}

static WRITE8_DEVICE_HANDLER( dio_w )
{
	/*

        bit     description

        PA0     DO1
        PA1     DO2
        PA2     DO3
        PA3     DO4
        PA4     DO5
        PA5     DO6
        PA6     DO7
        PA7     DO8

    */

	vic1112_t *vic1112 = get_safe_token(device->owner());

	ieee488_dio_w(vic1112->bus, device->owner(), data);
}

static WRITE_LINE_DEVICE_HANDLER( atn_w )
{
	vic1112_t *vic1112 = get_safe_token(device->owner());

	/* attention out */
	ieee488_atn_w(vic1112->bus, device->owner(), state);
}

static READ_LINE_DEVICE_HANDLER( srq_r )
{
	vic1112_t *vic1112 = get_safe_token(device->owner());

	/* service request in */
	return ieee488_srq_r(vic1112->bus);
}

static WRITE_LINE_DEVICE_HANDLER( eoi_w )
{
	vic1112_t *vic1112 = get_safe_token(device->owner());

	/* end or identify out */
	ieee488_eoi_w(vic1112->bus, device->owner(), state);
}

static const via6522_interface vic1112_via1_intf =
{
	DEVCB_NULL,
	DEVCB_HANDLER(dio_r),
	DEVCB_NULL,
	DEVCB_LINE(srq_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(dio_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(atn_w),
	DEVCB_LINE(eoi_w),

	DEVCB_LINE(via1_irq_w)
};

/*-------------------------------------------------
    MACHINE_DRIVER( vic1112 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( vic1112 )
	MCFG_VIA6522_ADD(M6522_0_TAG, 0, vic1112_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, 0, vic1112_via1_intf)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( vic1112 )
-------------------------------------------------*/

ROM_START( vic1112 )
	ROM_REGION( 0x800, VIC1112_TAG, ROMREGION_LOADBYNAME )
	ROM_LOAD( "325329-03.u2", 0x000, 0x800, CRC(d37b6335) SHA1(828c965829d21c60e8c2d083caee045c639a270f) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( vic1112 )
-------------------------------------------------*/

static DEVICE_START( vic1112 )
{
	vic1112_t *vic1112 = get_safe_token(device);
	const vic1112_config *config = get_safe_config(device);
	address_space *program = device->machine().firstcpu->memory().space(AS_PROGRAM);

	/* find devices */
	vic1112->via0 = device->subdevice<via6522_device>(M6522_0_TAG);
	vic1112->via1 = device->subdevice<via6522_device>(M6522_1_TAG);
	vic1112->bus = device->machine().device<via6522_device>(config->bus_tag);

	/* set VIA clocks */
	vic1112->via0->set_unscaled_clock(device->machine().firstcpu->unscaled_clock());
	vic1112->via1->set_unscaled_clock(device->machine().firstcpu->unscaled_clock());

	/* map VIAs to memory */
	program->install_readwrite_handler(0x9800, 0x980f, 0, 0, read8_delegate_create(via6522_device, read, *vic1112->via0), write8_delegate_create(via6522_device, write, *vic1112->via0));
	program->install_readwrite_handler(0x9810, 0x981f, 0, 0, read8_delegate_create(via6522_device, read, *vic1112->via1), write8_delegate_create(via6522_device, write, *vic1112->via1));

	/* map ROM to memory */
	program->install_rom(0xb000, 0xb7ff, device->machine().region("vic1112:vic1112")->base());

	/* ground REN */
	ieee488_ren_w(vic1112->bus, device, 0);

	/* register for state saving */
	device->save_item(NAME(vic1112->via0_irq));
	device->save_item(NAME(vic1112->via1_irq));
}

/*-------------------------------------------------
    DEVICE_RESET( vic1112 )
-------------------------------------------------*/

static DEVICE_RESET( vic1112 )
{
	vic1112_t *vic1112 = get_safe_token(device);

	/* reset VIAs */
	vic1112->via0->reset();
	vic1112->via1->reset();

	/* _IFC */
	ieee488_ifc_w(vic1112->bus, device, 0);
	ieee488_ifc_w(vic1112->bus, device, 1);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vic1112 )
-------------------------------------------------*/

DEVICE_GET_INFO( vic1112 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vic1112_t);								break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(vic1112_config);							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(vic1112);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(vic1112);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vic1112);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vic1112);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore VIC-1112");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-20");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(VIC1112, vic1112);
