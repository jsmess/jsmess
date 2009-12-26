/***************************************************************************

    QX-10

    Preliminary driver by Mariusz Wojcieszek

    Status:
    Driver boots and load CP/M from floppy image. Needs upd7220 for gfx
    and keyboard hooked to upd7021.

    Done:
    - preliminary memory map
    - floppy (upd765)
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
#include "machine/upd765.h"
#include "devices/messram.h"

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
	const device_config *upd765;
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
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu"));
	}
	else
	{
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + drambank*64*1024);
	}
	if (state->memcmos)
	{
		memory_set_bankptr(machine, "bank2", state->cmosram);
	}
	else
	{
		memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + drambank*64*1024 + 32*1024);
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

static WRITE_LINE_DEVICE_HANDLER(qx10_upd765_interrupt)
{
	qx10_state *driver_state = device->machine->driver_data;
	driver_state->fdcint = state;

	//logerror("Interrupt from upd765: %d\n", state);
	// signal interrupt
	pic8259_set_irq_line(driver_state->pic8259_master, 6, state);
};

static UPD765_DMA_REQUEST( drq_w )
{
	qx10_state *driver_state = device->machine->driver_data;
	//logerror("DMA Request from upd765: %d\n", state);
	i8237_dreq0_w(driver_state->dma8237_1, !state);
}

static const struct upd765_interface qx10_upd765_interface =
{
	DEVCB_LINE(qx10_upd765_interrupt),
	drq_w,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0,NULL, NULL, NULL}
};

static WRITE8_HANDLER(fdd_motor_w)
{
	qx10_state *driver_state = space->machine->driver_data;
	driver_state->fdcmotor = 1;

	floppy_mon_w(floppy_get_device(space->machine, 0), CLEAR_LINE);
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
static WRITE_LINE_DEVICE_HANDLER( dma_hrq_changed )
{
	/* Assert HLDA */
	i8237_hlda_w(device, state);
}

static READ8_DEVICE_HANDLER( gdc_dack_r )
{
	logerror("GDC DACK read\n");
	return 0;
}

static WRITE8_DEVICE_HANDLER( gdc_dack_w )
{
	logerror("GDC DACK write %02x\n", data);
}

static WRITE_LINE_DEVICE_HANDLER( tc_w )
{
	qx10_state *driver_state = device->machine->driver_data;

	/* floppy terminal count */
	upd765_tc_w(driver_state->upd765, !state);
}

/*
    8237 DMA (Master)
    Channel 1: Floppy disk
    Channel 2: GDC
    Channel 3: Option slots
*/
static I8237_INTERFACE( qx10_dma8237_1_interface )
{
	DEVCB_LINE(dma_hrq_changed),
	DEVCB_LINE(tc_w),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_write_byte),
	{ DEVCB_DEVICE_HANDLER("upd765", upd765_dack_r), DEVCB_HANDLER(gdc_dack_r),/*DEVCB_DEVICE_HANDLER("upd7220", upd7220_dack_r)*/ DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_DEVICE_HANDLER("upd765", upd765_dack_w), DEVCB_HANDLER(gdc_dack_w),/*DEVCB_DEVICE_HANDLER("upd7220", upd7220_dack_w)*/ DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};

/*
    8237 DMA (Slave)
    Channel 1: Option slots #1
    Channel 2: Option slots #2
    Channel 3: Option slots #3
    Channel 4: Option slots #4
*/
static I8237_INTERFACE( qx10_dma8237_2_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
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
    Counter CLK                         Gate                    OUT             Operation
    0       Keyboard clock (1200bps)    Memory register D0      Speaker timer   Speaker timer (100ms)
    1       Keyboard clock (1200bps)    +5V                     8259A (10E) IR5 Software timer
    2       Clock 1,9668MHz             Memory register D7      8259 (12E) IR1  Software timer
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
    Counter CLK                 Gate        OUT                 Operation
    0       Clock 1,9668MHz     +5V         Speaker frequency   1kHz
    1       Clock 1,9668MHz     +5V         Keyboard clock      1200bps (Clock / 1664)
    2       Clock 1,9668MHz     +5V         RS-232C baud rate   9600bps (Clock / 208)
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
    IR0     Power down detection interrupt
    IR1     Software timer #1 interrupt
    IR2     External interrupt INTF1
    IR3     External interrupt INTF2
    IR4     Keyboard/RS232 interrupt
    IR5     CRT/lightpen interrupt
    IR6     Floppy controller interrupt
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
    IR0     Printer interrupt
    IR1     External interrupt #1
    IR2     Calendar clock interrupt
    IR3     External interrupt #2
    IR4     External interrupt #3
    IR5     Software timer #2 interrupt
    IR6     External interrupt #4
    IR7     External interrupt #5

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
	AM_RANGE( 0x0000, 0x7fff ) AM_RAMBANK("bank1")
	AM_RANGE( 0x8000, 0xefff ) AM_RAMBANK("bank2")
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
	AM_RANGE(0x34, 0x34) AM_DEVREAD("upd765", upd765_status_r)
	AM_RANGE(0x35, 0x35) AM_DEVREADWRITE("upd765", upd765_data_r, upd765_data_w)
	AM_RANGE(0x38, 0x39) AM_READWRITE(compis_gdc_r, compis_gdc_w)
	AM_RANGE(0x3c, 0x3c) AM_READWRITE(mc146818_data_r, mc146818_data_w)
	AM_RANGE(0x3d, 0x3d) AM_WRITE(mc146818_offset_w)
	AM_RANGE(0x40, 0x4f) AM_DEVREADWRITE("8237dma_1", i8237_r, i8237_w)
	AM_RANGE(0x50, 0x5f) AM_DEVREADWRITE("8237dma_2", i8237_r, i8237_w)
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
	state->upd765 = devtag_get_device(machine, "upd765");

}

static MACHINE_RESET(qx10)
{
	qx10_state *state = machine->driver_data;

	i8237_dreq0_w(state->dma8237_1, 1);

	state->memprom = 0;
	state->memcmos = 0;
	state->membank = 0;
	update_memory_mapping(machine);
}

/* F4 Character Displayer */
static const gfx_layout qx10_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8, 8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( qx10 )
	GFXDECODE_ENTRY( "chargen", 0x0000, qx10_charlayout, 1, 7 )
GFXDECODE_END


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
	MDRV_I8237_ADD("8237dma_1", MAIN_CLK/4, qx10_dma8237_1_interface)
	MDRV_I8237_ADD("8237dma_2", MAIN_CLK/4, qx10_dma8237_2_interface)
	MDRV_UPD765A_ADD("upd765", qx10_upd765_interface)
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, qx10_floppy_config)

	MDRV_DRIVER_DATA(qx10_state)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_GFXDECODE(qx10)
	MDRV_PALETTE_LENGTH(COMPIS_PALETTE_SIZE)
	MDRV_PALETTE_INIT(compis_gdc)

	MDRV_VIDEO_START(compis_gdc)
	MDRV_VIDEO_UPDATE(compis_gdc)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( qx10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qx10boot.bin", 0x0000, 0x0800, CRC(f8dfcba5) SHA1(a7608f8aa7da355dcaf257ee28b66ded8974ce3a))

	/* This is probably the i8039 program ROM for the Q10MF Multifont card, and the actual font ROMs are missing (6 * HM43128) */
	/* The first part of this rom looks like code for an embedded controller?
        From 8300 on, looks like a characters generator */
	ROM_REGION( 0x800, "i8039", 0 )
	ROM_LOAD( "m12020a.3e", 0x0000, 0x0800, CRC(fa27f333) SHA1(73d27084ca7b002d5f370220d8da6623a6e82132))

	/* This rom is a character generator containing special characters only */
	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "qge.2e", 0x0000, 0x0800, CRC(ed93cb81) SHA1(579e68bde3f4184ded7d89b72c6936824f48d10b))
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

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, qx10,  0,       0, 	qx10, 	qx10, 	 qx10,  	   	  "Epson",   "QX-10",		GAME_NOT_WORKING)
