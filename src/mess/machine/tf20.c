/***************************************************************************

    Epson TF-20

    Dual floppy drive with HX-20 factory option

    Skeleton driver, not working

***************************************************************************/

#include "emu.h"
#include "tf20.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "machine/upd7201.h"
#include "machine/upd765.h"
#include "devices/flopdrv.h"


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
	running_device *ram;
	running_device *upd765a;
	running_device *upd7201;
	running_device *floppy_0;
	running_device *floppy_1;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE tf20_state *get_safe_token(running_device *device)
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
	tf20_state *tf20 = get_safe_token(space->cpu->owner());
	const address_space *prg = cpu_get_address_space(space->cpu, ADDRESS_SPACE_PROGRAM);

	/* switch in ram */
	memory_install_ram(prg, 0x0000, 0x7fff, 0, 0, messram_get_ptr(tf20->ram));

	/* clear tc */
	upd765_tc_w(tf20->upd765a, CLEAR_LINE);

	return 0xff;
}

static READ8_HANDLER( tf20_dip_r )
{
	tf20_state *tf20 = get_safe_token(space->cpu->owner());
	logerror("%s: tf20_dip_r\n", cpuexec_describe_context(space->machine));

	/* clear tc */
	upd765_tc_w(tf20->upd765a, CLEAR_LINE);

	return 0xff;
}

static READ8_DEVICE_HANDLER( tf20_upd765_tc_r )
{
	logerror("%s: tf20_upd765_tc_r\n", cpuexec_describe_context(device->machine));

	/* set tc on read */
	upd765_tc_w(device, ASSERT_LINE);

	return 0xff;
}

static WRITE8_HANDLER( tf20_fdc_control_w )
{
	tf20_state *tf20 = get_safe_token(space->cpu->owner());
	logerror("%s: tf20_fdc_control_w %02x\n", cpuexec_describe_context(space->machine), data);

	/* bit 0, motor on signal */
	floppy_mon_w(tf20->floppy_0, !BIT(data, 0));
	floppy_mon_w(tf20->floppy_1, !BIT(data, 0));
	floppy_drive_set_ready_state(tf20->floppy_0, BIT(data, 0), 1);
	floppy_drive_set_ready_state(tf20->floppy_1, BIT(data, 0), 1);

	/* set tc on write */
	upd765_tc_w(tf20->upd765a, ASSERT_LINE);
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
	logerror("%s: tf20_rxs_r\n", cpuexec_describe_context(device->machine));

	return upd7201_txda_r(tf20->upd7201);
}

READ_LINE_DEVICE_HANDLER( tf20_pins_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pins_r\n", cpuexec_describe_context(device->machine));

	return upd7201_dtra_r(tf20->upd7201);
}

/* serial input signal (from host computer) */
WRITE_LINE_DEVICE_HANDLER( tf20_txs_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_txs_w %u\n", cpuexec_describe_context(device->machine), state);

	upd7201_rxda_w(tf20->upd7201, state);
}

WRITE_LINE_DEVICE_HANDLER( tf20_pouts_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pouts_w %u\n", cpuexec_describe_context(device->machine), state);

	upd7201_ctsa_w(tf20->upd7201, state);
}

#ifdef UNUSED_FUNCTION
/* serial output signal (to another terminal) */
WRITE_LINE_DEVICE_HANDLER( tf20_txc_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_txc_w %u\n", cpuexec_describe_context(device->machine), state);

	upd7201_rxda_w(tf20->upd7201, state);
}

/* serial input signal (from another terminal) */
READ_LINE_DEVICE_HANDLER( tf20_rxc_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_rxc_r\n", cpuexec_describe_context(device->machine));

	return upd7201_txda_r(tf20->upd7201);
}

WRITE_LINE_DEVICE_HANDLER( tf20_poutc_w )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_poutc_w %u\n", cpuexec_describe_context(device->machine), state);

	upd7201_ctsa_w(tf20->upd7201, state);
}

READ_LINE_DEVICE_HANDLER( tf20_pinc_r )
{
	tf20_state *tf20 = get_safe_token(device);
	logerror("%s: tf20_pinc_r\n", cpuexec_describe_context(device->machine));

	return upd7201_dtra_r(tf20->upd7201);
}
#endif


/*****************************************************************************
    ADDRESS MAPS
*****************************************************************************/

static ADDRESS_MAP_START( tf20_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank21")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank22")
ADDRESS_MAP_END

static ADDRESS_MAP_START( tf20_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE("3a", upd7201_ba_cd_r, upd7201_ba_cd_w)
	AM_RANGE(0xf6, 0xf6) AM_READ(tf20_rom_disable)
	AM_RANGE(0xf7, 0xf7) AM_READ(tf20_dip_r)
	AM_RANGE(0xf8, 0xf8) AM_DEVREAD("5a", tf20_upd765_tc_r) AM_WRITE(tf20_fdc_control_w)
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("5a", upd765_status_r)
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("5a", upd765_data_r, upd765_data_w)
ADDRESS_MAP_END


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
	NULL,
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
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static MACHINE_DRIVER_START( tf20 )
	MDRV_CPU_ADD("tf20", Z80, XTAL_CR1 / 2) /* uPD780C */
	MDRV_CPU_PROGRAM_MAP(tf20_mem)
	MDRV_CPU_IO_MAP(tf20_io)

	/* 64k internal ram */
	MDRV_RAM_ADD("ram")
	MDRV_RAM_DEFAULT_SIZE("64k")

	/* upd765a floppy controller */
	MDRV_UPD765A_ADD("5a", tf20_upd765a_intf)

	/* upd7201 serial interface */
	MDRV_UPD7201_ADD("3a", XTAL_CR1 / 2, tf20_upd7201_intf)
	MDRV_TIMER_ADD_PERIODIC("serial_timer", serial_clock, HZ(XTAL_CR2 / 128))

	/* 2 floppy drives */
	MDRV_FLOPPY_2_DRIVES_ADD(tf20_floppy_config)
MACHINE_DRIVER_END


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
	running_device *cpu = device->subdevice("tf20");
	const address_space *prg = cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM);

	cpu_set_irq_callback(cpu, tf20_irq_ack);

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
	memory_install_ram(prg, 0x8000, 0xffff, 0, 0, messram_get_ptr(tf20->ram) + 0x8000);
}

static DEVICE_RESET( tf20 )
{
	running_device *cpu = device->subdevice("tf20");
	const address_space *prg = cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM);

	/* enable rom */
	memory_install_rom(prg, 0x0000, 0x07ff, 0, 0x7800, cpu->region()->base());
}

DEVICE_GET_INFO( tf20 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(tf20_state);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_DRIVER_NAME(tf20);	break;
		case DEVINFO_PTR_ROM_REGION:			info->romregion = ROM_NAME(tf20);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(tf20);			break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(tf20);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "TF-20");						break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Floppy drive");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEFINE_LEGACY_DEVICE(TF20, tf20);
