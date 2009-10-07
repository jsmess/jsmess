/***************************************************************************

	QX-10

	Preliminary driver by Mariusz Wojcieszek

	Status:
	Driver boots and load CP/M from floppy image. Needs upd7220 for gfx
	and keyboard hooked to upd7021.

	Done:
	- preliminary memory map
	- floppy (nec765)
	- DMA
	- Interrupts (pic8295)
****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/upd7201.h"
#include "machine/mc146818.h"
#include "machine/i8255a.h"
#include "machine/8237dma.h"
#include "video/i82720.h"
#include "machine/nec765.h"

#define MAIN_CLK	15974400

/*
	Driver data
*/

typedef struct _qx10_state qx10_state;
struct _qx10_state
{
	int		mc146818_offset;

	/* FDD */
	int		fdcint;
	int		fdcmotor;
	int		fdcready;

	/* memory */
	int		membank;
	int		memprom;
	int		memcmos;
	UINT8	cmosram[0x800];

	/* devices */
	const device_config *pic8259_master;
	const device_config *pic8259_slave;
	const device_config *dma8237_1;
	const device_config *nec765;
};

/*
	Memory
*/
static void update_memory_mapping(running_machine *machine)
{
	int drambank = 0;
	qx10_state *state = machine->driver_data;

	if (state->membank & 1)
	{
		drambank = 0;
	}
	else if (state->membank & 2)
	{
		drambank = 1;
	}
	else if (state->membank & 4)
	{
		drambank = 2;
	}
	else if (state->membank & 8)
	{
		drambank = 3;
	}

	if (!state->memprom)
	{
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu"));
	}
	else
	{
		memory_set_bankptr(machine, 1, mess_ram + drambank*64*1024);
	}
	if (state->memcmos)
	{
		memory_set_bankptr(machine, 2, state->cmosram);
	}
	else
	{
		memory_set_bankptr(machine, 2, mess_ram + drambank*64*1024 + 32*1024);
	}
}

static WRITE8_HANDLER(qx10_18_w)
{
	qx10_state *state = space->machine->driver_data;
	state->membank = (data >> 4) & 0x0f;
	update_memory_mapping(space->machine);
}

static WRITE8_HANDLER(prom_sel_w)
{
	qx10_state *state = space->machine->driver_data;
	state->memprom = data & 1;
	update_memory_mapping(space->machine);
}

static WRITE8_HANDLER(cmos_sel_w)
{
	qx10_state *state = space->machine->driver_data;
	state->memcmos = data & 1;
	update_memory_mapping(space->machine);
}

/*
	FDD
*/

static const floppy_config qx10_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

static WRITE_LINE_DEVICE_HANDLER(qx10_nec765_interrupt)
{
	qx10_state *driver_state = device->machine->driver_data;
	driver_state->fdcint = state;

	//logerror("Interrupt from nec765: %d\n", state);
	// signal interrupt
	pic8259_set_irq_line(driver_state->pic8259_master, 6, state);
};

static NEC765_DMA_REQUEST( drq_w )
{
	qx10_state *driver_state = device->machine->driver_data;
	//logerror("DMA Request from nec765: %d\n", state);
	dma8237_drq_write(driver_state->dma8237_1, 0, !state);
}

static const struct nec765_interface qx10_nec765_interface =
{
	DEVCB_LINE(qx10_nec765_interrupt),
	drq_w,
	NULL,
	NEC765_RDY_PIN_CONNECTED,
	{FLOPPY_0,NULL, NULL, NULL}
};

static WRITE8_HANDLER(fdd_motor_w)
{
	qx10_state *driver_state = space->machine->driver_data;
	driver_state->fdcmotor = 1;

	floppy_drive_set_motor_state(floppy_get_device(space->machine, 0), 1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,1);
	// motor off controlled by clock
};

static READ8_HANDLER(qx10_30_r)
{
	qx10_state *driver_state = space->machine->driver_data;
	return driver_state->fdcint |
		   /*driver_state->fdcmotor*/ 0 << 1 |
		   driver_state->membank << 4;
};

/*
	DMA8237
*/
static DMA8237_HRQ_CHANGED( dma_hrq_changed )
{
	/* Assert HLDA */
	dma8237_set_hlda(device, state);
}

static DMA8237_MEM_READ( memory_dma_r )
{
	const address_space *program = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	//logerror("DMA read %04x\n", offset);
	return memory_read_byte(program, offset);
}

static DMA8237_MEM_WRITE( memory_dma_w )
{
	const address_space *program = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	//logerror("DMA write %04x:%02x\n", offset, data);
	memory_write_byte(program, offset, data);
}

static DMA8237_CHANNEL_READ( gdc_dack_r )
{
	logerror("GDC DACK read\n");
	return 0;
}

static DMA8237_CHANNEL_WRITE( gdc_dack_w )
{
	logerror("GDC DACK write %02x\n", data);
}

static DMA8237_CHANNEL_READ( fdc_dack_r )
{
	qx10_state *state = device->machine->driver_data;
	UINT8 data = nec765_dack_r(state->nec765, 0);
	//logerror("FDC DACK read %02x\n", data);
	return data;
}

static DMA8237_CHANNEL_WRITE( fdc_dack_w )
{
	qx10_state *state = device->machine->driver_data;
	//logerror("FDC DACK write %02x\n", data);

	nec765_dack_w(state->nec765, 0, data);
}

static DMA8237_OUT_EOP( tc_w )
{
	qx10_state *driver_state = device->machine->driver_data;

	/* floppy terminal count */
	nec765_tc_w(driver_state->nec765, !state);
}

/*
	8237 DMA (Master)
	Channel 1: Floppy disk
	Channel 2: GDC
	Channel 3: Option slots
*/
static const struct dma8237_interface qx10_dma8237_1_interface =
{
	MAIN_CLK/4,		/* speed of DMA accesses (per byte) */
	dma_hrq_changed,/* function that will be called when HRQ may have changed */
	memory_dma_r,	/* accessors to main memory */
	memory_dma_w,

	{ fdc_dack_r, gdc_dack_r, NULL, NULL },	/* channel accesors */
	{ fdc_dack_w, gdc_dack_w, NULL, NULL },

	tc_w			/* function to call when DMA completes */
};

/*
	8237 DMA (Slave)
	Channel 1: Option slots #1
	Channel 2: Option slots #2
	Channel 3: Option slots #3
	Channel 4: Option slots #4
*/
static const struct dma8237_interface qx10_dma8237_2_interface =
{
	MAIN_CLK/4,		/* speed of DMA accesses (per byte) */
	NULL,			/* function that will be called when HRQ may have changed */
	NULL,			/* accessors to main memory */
	NULL,

	{ NULL, NULL, NULL, NULL },	/* channel accesors */
	{ NULL, NULL, NULL, NULL },

	NULL			/* function to call when DMA completes */
};

/*
	8255
*/
static I8255A_INTERFACE(qx10_i8255_interface)
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/*
	MC146818
*/
static READ8_HANDLER(mc146818_data_r)
{
	qx10_state *state = space->machine->driver_data;
	return mc146818_port_r(space, state->mc146818_offset);
};

static WRITE8_HANDLER(mc146818_data_w)
{
	qx10_state *state = space->machine->driver_data;
	mc146818_port_w(space, state->mc146818_offset, data);
};

static WRITE8_HANDLER(mc146818_offset_w)
{
	qx10_state *state = space->machine->driver_data;
	state->mc146818_offset = data;
};

/*
	UPD7201
	Channel A: Keyboard
	Channel B: RS232
*/
static UPD7201_INTERFACE(qx10_upd7201_interface)
{
	DEVCB_NULL,					/* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}, {
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/*
	Timer 0
	Counter	CLK							Gate					OUT				Operation
	0		Keyboard clock (1200bps)	Memory register D0		Speaker timer	Speaker timer (100ms)
	1		Keyboard clock (1200bps)	+5V						8259A (10E) IR5	Software timer
	2		Clock 1,9668MHz				Memory register D7		8259 (12E) IR1	Software timer
*/

static const struct pit8253_config qx10_pit8253_1_config =
{
	{
		{ 1200, NULL },
		{ 1200, NULL },
		{ MAIN_CLK / 8, NULL },
	}
};

/*
 	Timer 1
	Counter	CLK					Gate		OUT					Operation
	0		Clock 1,9668MHz		+5V			Speaker frequency	1kHz
	1		Clock 1,9668MHz		+5V			Keyboard clock		1200bps (Clock / 1664)
	2		Clock 1,9668MHz		+5V			RS-232C baud rate	9600bps (Clock / 208)
*/
static const struct pit8253_config qx10_pit8253_2_config =
{
	{
		{ MAIN_CLK / 8, NULL },
		{ MAIN_CLK / 8, NULL },
		{ MAIN_CLK / 8, NULL },
	}
};


/*
	Master PIC8259
	IR0		Power down detection interrupt
	IR1		Software timer #1 interrupt
	IR2		External interrupt INTF1
	IR3		External interrupt INTF2
	IR4		Keyboard/RS232 interrupt
	IR5		CRT/lightpen interrupt
	IR6		Floppy controller interrupt
*/

static PIC8259_SET_INT_LINE( qx10_pic8259_master_set_int_line )
{
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static const struct pic8259_interface qx10_pic8259_master_config =
{
	qx10_pic8259_master_set_int_line
};

/*
	Slave PIC8259
	IR0		Printer interrupt
	IR1		External interrupt #1
	IR2		Calendar clock interrupt
	IR3		External interrupt #2
	IR4		External interrupt #3
	IR5		Software timer #2 interrupt
	IR6		External interrupt #4
	IR7		External interrupt #5

*/

static PIC8259_SET_INT_LINE( qx10_pic8259_slave_set_int_line )
{
	pic8259_set_irq_line(((qx10_state*)device->machine->driver_data)->pic8259_master, 7, interrupt);
}

static const struct pic8259_interface qx10_pic8259_slave_config =
{
	qx10_pic8259_slave_set_int_line
};

static IRQ_CALLBACK(irq_callback)
{
	int r = 0;
	r = pic8259_acknowledge( ((qx10_state*)device->machine->driver_data)->pic8259_slave );
	if (r==0)
	{
		r = pic8259_acknowledge( ((qx10_state*)device->machine->driver_data)->pic8259_master );
	}
	return r;
}

static ADDRESS_MAP_START(qx10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAMBANK(1)
	AM_RANGE( 0x8000, 0xefff ) AM_RAMBANK(2)
	AM_RANGE( 0xf000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qx10_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("pit8253_1", pit8253_r, pit8253_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("pit8253_2", pit8253_r, pit8253_w)
	AM_RANGE(0x08, 0x09) AM_DEVREADWRITE("pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE(0x0c, 0x0d) AM_DEVREADWRITE("pic8259_slave", pic8259_r, pic8259_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE("upd7201", upd7201_cd_ba_r, upd7201_cd_ba_w)
	AM_RANGE(0x14, 0x17) AM_DEVREADWRITE("i8255", i8255a_r, i8255a_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(qx10_18_w)
	AM_RANGE(0x1c, 0x1c) AM_WRITE(prom_sel_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(cmos_sel_w)
	AM_RANGE(0x2c, 0x2c) AM_READ_PORT("CONFIG")
	AM_RANGE(0x30, 0x30) AM_READWRITE(qx10_30_r, fdd_motor_w)
	AM_RANGE(0x34, 0x34) AM_DEVREAD("nec765", nec765_status_r)
	AM_RANGE(0x35, 0x35) AM_DEVREADWRITE("nec765", nec765_data_r, nec765_data_w)
	AM_RANGE(0x38, 0x39) AM_READWRITE(compis_gdc_r, compis_gdc_w)
	AM_RANGE(0x3c, 0x3c) AM_READWRITE(mc146818_data_r, mc146818_data_w)
	AM_RANGE(0x3d, 0x3d) AM_WRITE(mc146818_offset_w)
	AM_RANGE(0x40, 0x4f) AM_DEVREADWRITE("8237dma_1", dma8237_r, dma8237_w)
	AM_RANGE(0x50, 0x5f) AM_DEVREADWRITE("8237dma_2", dma8237_r, dma8237_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( qx10 )
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x03, 0x02, "Video Board" )
	PORT_CONFSETTING( 0x02, "Monochrome" )
	PORT_CONFSETTING( 0x01, "Color" )
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/*
	Video
*/
static const compis_gdc_interface i82720_interface =
{
	GDC_MODE_HRG,
	0x8000
};

static MACHINE_START(qx10)
{
	qx10_state *state = machine->driver_data;

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), irq_callback);

	mc146818_init(machine, MC146818_STANDARD);
	compis_init( &i82720_interface );

	// find devices
	state->pic8259_master = devtag_get_device(machine, "pic8259_master");
	state->pic8259_slave = devtag_get_device(machine, "pic8259_slave");
	state->dma8237_1 = devtag_get_device(machine, "8237dma_1");
	state->nec765 = devtag_get_device(machine, "nec765");

}

static MACHINE_RESET(qx10)
{
	qx10_state *state = machine->driver_data;

	dma8237_drq_write(state->dma8237_1, 0, 1);

	state->memprom = 0;
	state->memcmos = 0;
	state->membank = 0;
	update_memory_mapping(machine);
}

static MACHINE_DRIVER_START( qx10 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, MAIN_CLK / 4)
    MDRV_CPU_PROGRAM_MAP(qx10_mem)
    MDRV_CPU_IO_MAP(qx10_io)

	MDRV_MACHINE_START(qx10)
    MDRV_MACHINE_RESET(qx10)

	MDRV_PIT8253_ADD("pit8253_1", qx10_pit8253_1_config)
	MDRV_PIT8253_ADD("pit8253_2", qx10_pit8253_2_config)
	MDRV_PIC8259_ADD("pic8259_master", qx10_pic8259_master_config)
	MDRV_PIC8259_ADD("pic8259_slave", qx10_pic8259_slave_config)
	MDRV_UPD7201_ADD("upd7201", MAIN_CLK/4, qx10_upd7201_interface)
	MDRV_I8255A_ADD("i8255", qx10_i8255_interface)
	MDRV_DMA8237_ADD("8237dma_1", qx10_dma8237_1_interface)
	MDRV_DMA8237_ADD("8237dma_2", qx10_dma8237_2_interface)
	MDRV_NEC765A_ADD("nec765", qx10_nec765_interface)
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, qx10_floppy_config)

	MDRV_DRIVER_DATA(qx10_state)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(COMPIS_PALETTE_SIZE)
    MDRV_PALETTE_INIT(compis_gdc)

    MDRV_VIDEO_START(compis_gdc)
    MDRV_VIDEO_UPDATE(compis_gdc)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(qx10)
	CONFIG_RAM_DEFAULT(256 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( qx10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qx10boot.bin", 0x0000, 0x0800, CRC(f8dfcba5) SHA1(a7608f8aa7da355dcaf257ee28b66ded8974ce3a))
	/* The first part of this rom looks like code for an embedded controller?
        From 8300 on, looks like a characters generator */
	ROM_LOAD( "mfboard.bin", 0x8000, 0x0800, CRC(fa27f333) SHA1(73d27084ca7b002d5f370220d8da6623a6e82132))
	/* This rom looks like a character generator */
	ROM_LOAD( "qge.bin", 0x8800, 0x0800, CRC(ed93cb81) SHA1(579e68bde3f4184ded7d89b72c6936824f48d10b))
ROM_END

/* Driver */

static DRIVER_INIT(qx10)
{
	// patch boot rom
	UINT8 *bootrom = memory_region(machine, "maincpu");
	bootrom[0x250] = 0x00; /* nop */
	bootrom[0x251] = 0x00; /* nop */
	bootrom[0x252] = 0x00; /* nop */
	bootrom[0x253] = 0x00; /* nop */
	bootrom[0x254] = 0x00; /* nop */
	bootrom[0x255] = 0x00; /* nop */
	bootrom[0x256] = 0x00; /* nop */
	bootrom[0x257] = 0x00; /* nop */
	bootrom[0x258] = 0x00; /* nop */
	bootrom[0x259] = 0x00; /* nop */
	bootrom[0x25a] = 0x00; /* nop */
	bootrom[0x25b] = 0x00; /* nop */
	bootrom[0x25c] = 0x00; /* nop */
	bootrom[0x25d] = 0x00; /* nop */
	bootrom[0x25e] = 0x3e; /* ld a,11 */
	bootrom[0x25f] = 0x11;

}

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, qx10,  0,       0, 	qx10, 	qx10, 	 qx10,  	  qx10,  	 "Epson",   "QX-10",		GAME_NOT_WORKING)
