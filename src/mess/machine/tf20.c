/***************************************************************************

    Epson TF-20

    Dual floppy drive with HX-20 factory option


    Status: Boots from system disk, missing ??PD7201 emulation

***************************************************************************/

#include "emu.h"
#include "tf20.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "machine/upd7201.h"
#include "machine/upd765.h"
#include "imagedev/flopdrv.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define XTAL_CR1	XTAL_8MHz
#define XTAL_CR2	XTAL_4_9152MHz


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tf20_state tf20_state;
struct _tf20_state
{
	device_t *ram;
	device_t *upd765a;
	device_t *upd7201;
	device_t *floppy_0;
	device_t *floppy_1;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE tf20_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TF20);

	return (tf20_state *)downcast<legacy_device_base *>(device)->token();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* serial clock, 38400 baud by default */
static TIMER_DEVICE_CALLBACK( serial_clock )
{
	tf20_state *tf20 = get_safe_token(timer.owner());

	upd7201_rxca_w(tf20->upd7201, ASSERT_LINE);
	upd7201_txca_w(tf20->upd7201, ASSERT_LINE);
	upd7201_rxcb_w(tf20->upd7201, ASSERT_LINE);
	upd7201_txcb_w(tf20->upd7201, ASSERT_LINE);
}

/* a read from this location disables the rom */
static READ8_HANDLER( tf20_rom_disable )
{
	tf20_state *tf20 = get_safe_token(space->device().owner());
	address_space *prg = space->device().memory().space(AS_PROGRAM);

	/* switch in ram */
	prg->install_ram(0x0000, 0x7fff, ram_get_ptr(tf20->ram));

	return 0xff;
}

static READ8_HANDLER( tf20_dip_r )
{
	logerror("%s: tf20_dip_r\n", space->machine().describe_context());

	return input_port_read(space->machine(), "tf20_dip");
}

static TIMER_CALLBACK( tf20_upd765_tc_reset )
{
	upd765_tc_w((device_t *)ptr, CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( tf20_upd765_tc_r )
{
	logerror("%s: tf20_upd765_tc_r\n", device->machine().describe_context());

	/* toggle tc on read */
	upd765_tc_w(device, ASSERT_LINE);
	device->machine().scheduler().timer_set(attotime::zero, FUNC(tf20_upd765_tc_reset), 0, device);

	return 0xff;
}

static WRITE8_HANDLER( tf20_fdc_control_w )
{
	tf20_state *tf20 = get_safe_token(space->device().owner());
	logerror("%s: tf20_fdc_control_w %02x\n", space->machine().describe_context(), data);

	/* bit 0, motor on signal */
	floppy_mon_w(tf20->floppy_0, !BIT(data, 0));
	floppy_mon_w(tf20->floppy_1, !BIT(data, 0));
}

static IRQ_CALLBACK( tf20_irq_ack )
{
	return 0x00;
}


/***************************************************************************
    EXTERNAL INTERFACE
***************************************************************************/

/* serial output signal (to the host computer) */
READ_LINE_DEVICE_HANDLER( tf20_rxs_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_rxs_r\n", device->machine().describe_context());

	return upd7201_txda_r(tf20->upd7201);
}

READ_LINE_DEVICE_HANDLER( tf20_pins_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pins_r\n", device->machine().describe_context());

	return upd7201_dtra_r(tf20->upd7201);
}

/* serial input signal (from host computer) */
WRITE_LINE_DEVICE_HANDLER( tf20_txs_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_txs_w %u\n", device->machine().describe_context(), state);

	upd7201_rxda_w(tf20->upd7201, state);
}

WRITE_LINE_DEVICE_HANDLER( tf20_pouts_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pouts_w %u\n", device->machine().describe_context(), state);

	upd7201_ctsa_w(tf20->upd7201, state);
}

#ifdef UNUSED_FUNCTION
/* serial output signal (to another terminal) */
WRITE_LINE_DEVICE_HANDLER( tf20_txc_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_txc_w %u\n", device->machine().describe_context(), state);

	upd7201_rxda_w(tf20->upd7201, state);
}

/* serial input signal (from another terminal) */
READ_LINE_DEVICE_HANDLER( tf20_rxc_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_rxc_r\n", device->machine().describe_context());

	return upd7201_txda_r(tf20->upd7201);
}

WRITE_LINE_DEVICE_HANDLER( tf20_poutc_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_poutc_w %u\n", device->machine().describe_context(), state);

	upd7201_ctsa_w(tf20->upd7201, state);
}

READ_LINE_DEVICE_HANDLER( tf20_pinc_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pinc_r\n", device->machine().describe_context());

	return upd7201_dtra_r(tf20->upd7201);
}
#endif


/*****************************************************************************
    ADDRESS MAPS
*****************************************************************************/

static ADDRESS_MAP_START( tf20_mem, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank21")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank22")
ADDRESS_MAP_END

static ADDRESS_MAP_START( tf20_io, AS_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE("3a", upd7201_ba_cd_r, upd7201_ba_cd_w)
	AM_RANGE(0xf6, 0xf6) AM_READ(tf20_rom_disable)
	AM_RANGE(0xf7, 0xf7) AM_READ(tf20_dip_r)
	AM_RANGE(0xf8, 0xf8) AM_DEVREAD("5a", tf20_upd765_tc_r) AM_WRITE(tf20_fdc_control_w)
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("5a", upd765_status_r)
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("5a", upd765_data_r, upd765_data_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

INPUT_PORTS_START( tf20 )
	PORT_START("tf20_dip")
	PORT_DIPNAME(0x0f, 0x0f, "Drive extension")
	PORT_DIPLOCATION("TF-20 TFX:8,7,6,5")
	PORT_DIPSETTING(0x0f, "A & B Drive")
	PORT_DIPSETTING(0x07, "C & D Drive")
INPUT_PORTS_END


/*****************************************************************************
    MACHINE CONFIG
*****************************************************************************/

static UPD7201_INTERFACE( tf20_upd7201_intf )
{
	DEVCB_NULL,				/* interrupt: nc */
	{
		{
			XTAL_CR2 / 128,		/* receive clock: 38400 baud (default) */
			XTAL_CR2 / 128,		/* transmit clock: 38400 baud (default) */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output: nc */
		}, {
			XTAL_CR2 / 128,		/* receive clock: 38400 baud (default) */
			XTAL_CR2 / 128,		/* transmit clock: 38400 baud (default) */
			DEVCB_NULL,			/* receive DRQ: nc */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_LINE_GND,		/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready: nc */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output: nc */
		}
	}
};

static const upd765_interface tf20_upd765a_intf =
{
	DEVCB_CPU_INPUT_LINE("tf20", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static const floppy_config tf20_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD_40,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static MACHINE_CONFIG_FRAGMENT( tf20 )
	MCFG_CPU_ADD("tf20", Z80, XTAL_CR1 / 2) /* uPD780C */
	MCFG_CPU_PROGRAM_MAP(tf20_mem)
	MCFG_CPU_IO_MAP(tf20_io)

	/* 64k internal ram */
	MCFG_RAM_ADD("ram")
	MCFG_RAM_DEFAULT_SIZE("64k")

	/* upd765a floppy controller */
	MCFG_UPD765A_ADD("5a", tf20_upd765a_intf)

	/* upd7201 serial interface */
	MCFG_UPD7201_ADD("3a", XTAL_CR1 / 2, tf20_upd7201_intf)
	MCFG_TIMER_ADD_PERIODIC("serial_timer", serial_clock, attotime::from_hz(XTAL_CR2 / 128))

	/* 2 floppy drives */
	MCFG_FLOPPY_2_DRIVES_ADD(tf20_floppy_config)
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( tf20 )
	ROM_REGION(0x0800, "tf20", ROMREGION_LOADBYNAME)
	ROM_LOAD("tfx.15e", 0x0000, 0x0800, CRC(af34f084) SHA1(c9bdf393f757ba5d8f838108ceb2b079be1d616e))
ROM_END


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( tf20 )
{
	tf20_state *tf20 = get_safe_token(device);
	device_t *cpu = device->subdevice("tf20");
	address_space *prg = cpu->memory().space(AS_PROGRAM);

	device_set_irq_callback(cpu, tf20_irq_ack);

	/* ram device */
	tf20->ram = device->subdevice("ram");

	/* make sure its already running */
	if (!tf20->ram->started())
		throw device_missing_dependencies();

	/* locate child devices */
	tf20->upd765a = device->subdevice("5a");
	tf20->upd7201 = device->subdevice("3a");
	tf20->floppy_0 = device->subdevice(FLOPPY_0);
	tf20->floppy_1 = device->subdevice(FLOPPY_1);

	/* enable second half of ram */
	prg->install_ram(0x8000, 0xffff, ram_get_ptr(tf20->ram) + 0x8000);
}

static DEVICE_RESET( tf20 )
{
	device_t *cpu = device->subdevice("tf20");
	address_space *prg = cpu->memory().space(AS_PROGRAM);

	/* enable rom */
	prg->install_rom(0x0000, 0x07ff, 0, 0x7800, cpu->region()->base());
}

DEVICE_GET_INFO( tf20 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(tf20_state);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_CONFIG_NAME(tf20);	break;
		case DEVINFO_PTR_ROM_REGION:			info->romregion = ROM_NAME(tf20);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(tf20);			break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(tf20);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "TF-20");						break;
		case DEVINFO_STR_SHORTNAME:				strcpy(info->s, "tf20");						break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Floppy drive");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEFINE_LEGACY_DEVICE(TF20, tf20);
