/*

Luxor Conkort

PCB Layout
----------

?

|-----------------------------------|
|   LD1 SW1 LS132       CON2        |
|                               S   |
|   4MHz        S240    N8T97N      |
|   S                               |
|   7404            MC1458  4024    |
|       S                           |
|   74276   S                       |
|           S   S                   |
|   LS32    S   C140E       LS273   |
|                                   |
|                                   |
|       MB8876          ROM         |
|                                   |
|                                   |
|   S   LS32    LS156       TC5514  |
|                                   |
|                           TC5514  |
|       Z80                         |
|                           LS273   |
|                                   |
|                           LS373   |
|       Z80PIO                      |
|                           LS245   |
|                                   |
|   LS125   LS124       S   DM8131  |
|                                   |
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM     - Hitachi HN462716 2Kx8 EPROM "MPT02"
    Z80     - Sharp LH0080A Z80A CPU
    Z80PIO  - SGS Z8420AB1 Z80A PIO
    MB8876  - Mitsubishi MB8876 Floppy Disc Controller (FD1791 compatible)
    TC5514  - Toshiba TC5514AP-2 1Kx4 bit Static RAM
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    C140E   - Ferranti ?C140E "copy protection device"
    N8T97N  - SA N8T97N ?
    CON1    - ABC bus connector
    CON2    - 25-pin D sub floppy connector (AMP4284)
    SW1     - Disk drive type (SS/DS, SD/DD)
    S1      -
    S2      -
    S3      -
    S4      -
    S5      -
    S6      -
    S7      -
    S8      -
    S9      -
    LD1     -

PCB Layout
----------

55 11046-03

|-----------------------------------|
|   LD1 SW1             CON2        |
|                                   |
|                       CON3    S8  |
|                               S9  |
|   LS240   LS174   7406    7406    |
|                                   |
|   LS174   FDC9229 LS107   LS266   |
|                                   |
|                                   |
|       SAB1793                     |
|                   LS368   16MHz   |
|                   S6              |
|       Z80DMA          ROM         |
|                                   |
|                                   |
|       Z80             TC5565      |
|                                   |
|                                   |
|   LS138   LS174   SW2     74374   |
|                                   |
|   LS10    LS266   S5      LS374   |
|                   S3              |
|                   S1      LS240   |
|                                   |
|           LS244   SW3     DM8131  |
|                                   |
|                                   |
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM     - Toshiba TMM27128D-20 16Kx8 EPROM "CNTR 1.07 6490318-07"
    TC5565  - Toshiba TC5565PL-15 8Kx8 bit Static RAM
    Z80     - Zilog Z8400APS Z80A CPU
    Z80DMA  - Zilog Z8410APS Z80A DMA
    SAB1793 - Siemens SAB1793-02P Floppy Disc Controller
    FDC9229 - SMC FDC9229BT Floppy Disc Interface Circuit
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    CON1    - ABC bus connector
    CON2    - 25-pin D sub floppy connector (AMP4284)
    CON3    - 34-pin header floppy connector
    SW1     - Disk drive type (SS/DS, SD/DD)
    SW2     - Disk drive model
    SW3     - ABC bus address
    S1      - Interface type (A:? B:ABCBUS)
    S3      - Interface type (A:? B:ABCBUS)
    S5      - Interface type (A:? B:ABCBUS)
    S6      - Amount of RAM installed (A:2KB, B:8KB)
    S7      - Number of drives connected (0:3, 1:2) *located on solder side
    S8      - Disk drive type (0:8", 1:5.25")
    S9      - Location of RDY signal (A:8" P2-6, B:5.25" P2-34)
    LD1     - LED

*/

/*

    TODO:

    Slow Controller
    ---------------

    - Z80 IN instruction needs to halt in mid air for this controller to ever work (the first data byte of disk sector is read too early)

        wd17xx_command_w $88 READ_SEC
        wd17xx_data_r: (no new data) $00 (data_count 0)
        WAIT
        wd179x: Read Sector callback.
        sector found! C:$00 H:$00 R:$0b N:$01
        wd17xx_data_r: $FF (data_count 256)
        WAIT

    - copy protection device (sends sector header bytes to CPU? DDEN is serial clock? code checks for either $b6 or $f7)

        06F8: ld   a,$2F                    ; SEEK
        06FA: out  ($BC),a
        06FC: push af
        06FD: push bc
        06FE: ld   bc,$0724
        0701: push bc
        0702: ld   b,$07
        0704: rr   a
        0706: call $073F
        073F: DB 7E         in   a,($7E)    ; PIO PORT B
        0741: EE 08         xor  $08        ; DDEN
        0743: D3 7E         out  ($7E),a
        0745: EE 08         xor  $08
        0747: D3 7E         out  ($7E),a
        0749: DB 7E         in   a,($7E)
        074B: 1F            rra
        074C: 1F            rra
        074D: 1F            rra
        074E: CB 11         rl   c
        0750: 79            ld   a,c
        0751: C9            ret
        0709: djnz $0703            <-- jumps to middle of instruction!
        0703: rlca
        0704: rr   a
        0706: call $073F

    - FD1791 HLD/HLT callbacks
    - DS/DD SS/DS jumpers
    - S1-S5 jumpers

    Fast Controller
    ---------------
	- status bit 0

*/

#include "emu.h"
#include "conkort.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/wd17xx.h"
#include "machine/abcbus.h"
#include "devices/flopdrv.h"
#include "formats/flopimg.h"
#include "formats/basicdsk.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define Z80_TAG		"5a"
#define Z80PIO_TAG	"3a"
#define FD1791_TAG	"7a"

//#define Z80_TAG	"5ab"
#define Z80DMA_TAG	"6ab"
#define SAB1793_TAG	"7ab"
#define FDC9229_TAG	"8b"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _slow_t slow_t;
struct _slow_t
{
	int cs;					/* card selected */
	UINT8 status;			/* ABC BUS status */
	UINT8 data;				/* ABC BUS data */
	int fdc_irq;			/* floppy interrupt */
	int fdc_drq;			/* floppy data request */
	int wait_enable;		/* wait enable */

	/* devices */
	running_device *cpu;
	running_device *z80pio;
	running_device *fd1791;
	running_device *image0;
	running_device *image1;
};

typedef struct _fast_t fast_t;
struct _fast_t
{
	int cs;					/* card selected */
	UINT8 status;			/* ABC BUS status */
	UINT8 data_in;			/* ABC BUS data in */
	UINT8 data_out;			/* ABC BUS data out */
	int fdc_irq;			/* FDC interrupt */

	/* devices */
	running_device *cpu;
	running_device *z80dma;
	running_device *wd1793;
	running_device *image0;
	running_device *image1;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE slow_t *get_safe_token_slow(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == LUXOR_55_10828);
	return (slow_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE fast_t *get_safe_token_fast(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == LUXOR_55_21046);
	return (fast_t *)downcast<legacy_device_base *>(device)->token();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* Slow Controller */

WRITE8_DEVICE_HANDLER( luxor_55_10828_cs_w )
{
	slow_t *conkort = get_safe_token_slow(device);

	conkort->cs = (data == 0x2d); // TODO: bit 0 of this is configurable with S1
}

READ8_DEVICE_HANDLER( luxor_55_10828_inp_r )
{
	slow_t *conkort = get_safe_token_slow(device);
	UINT8 data = 0xff;

	if (conkort->cs)
	{
		if (!BIT(conkort->status, 6))
		{
			data = conkort->data;
		}

		z80pio_astb_w(conkort->z80pio, 0);
		z80pio_astb_w(conkort->z80pio, 1);
	}

	return data;
}

WRITE8_DEVICE_HANDLER( luxor_55_10828_utp_w )
{
	slow_t *conkort = get_safe_token_slow(device);

	if (!conkort->cs) return;

	if (BIT(conkort->status, 6))
	{
		conkort->data = data;
	}

	z80pio_astb_w(conkort->z80pio, 0);
	z80pio_astb_w(conkort->z80pio, 1);
}

READ8_DEVICE_HANDLER( luxor_55_10828_stat_r )
{
	slow_t *conkort = get_safe_token_slow(device);
	UINT8 data = 0xff;

	if (conkort->cs)
	{
		data = (conkort->status & 0xfe) | z80pio_ardy_r(conkort->z80pio);
	}

	return data;
}

WRITE8_DEVICE_HANDLER( luxor_55_10828_c1_w )
{
	slow_t *conkort = get_safe_token_slow(device);

	if (conkort->cs)
	{
		cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, ASSERT_LINE);
		cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, CLEAR_LINE);
	}
}

WRITE8_DEVICE_HANDLER( luxor_55_10828_c3_w )
{
	slow_t *conkort = get_safe_token_slow(device);

	if (conkort->cs)
	{
		cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
	}
}

WRITE_LINE_DEVICE_HANDLER( luxor_55_10828_rst_w )
{
	if (!state)
	{
		device->reset();
	}
}

static WRITE8_DEVICE_HANDLER( slow_ctrl_w )
{
	/*

        bit     signal          description

        0       SEL 0
        1       SEL 1
        2       SEL 2
        3       _MOT ON
        4       SIDE
        5       _PRECOMP ON
        6       _WAIT ENABLE
        7       FDC _MR

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	/* drive selection */
	if (BIT(data, 0)) wd17xx_set_drive(device, 0);
	if (BIT(data, 1)) wd17xx_set_drive(device, 1);
//  if (BIT(data, 2)) wd17xx_set_drive(device, 2);

	/* motor enable */
	floppy_mon_w(conkort->image0, BIT(data, 3));
	floppy_mon_w(conkort->image1, BIT(data, 3));
	floppy_drive_set_ready_state(conkort->image0, 1, 1);
	floppy_drive_set_ready_state(conkort->image1, 1, 1);

	/* disk side selection */
//  wd17xx_set_side(device, BIT(data, 4));

	/* wait enable */
	conkort->wait_enable = BIT(data, 6);

	/* FDC master reset */
	wd17xx_mr_w(device, BIT(data, 7));

	logerror("CTRL %02x\n", data);
}

static WRITE8_DEVICE_HANDLER( slow_status_w )
{
	/*

        bit     description

        0       _INT to main Z80
        1
        2
        3
        4
        5
        6       LS245 DIR
        7

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->status = data;
}

static READ8_DEVICE_HANDLER( slow_fdc_r )
{
	slow_t *conkort = get_safe_token_slow(device->owner());
	UINT8 data = 0xff;

	if (!conkort->wait_enable && !conkort->fdc_irq && !conkort->fdc_drq)
	{
		/* TODO: this is really connected to the Z80 WAIT line */
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	switch (offset & 0x03)
	{
	case 0: data = wd17xx_status_r(device, 0);
	case 1:	data = wd17xx_track_r(device, 0);
	case 2:	data = wd17xx_sector_r(device, 0);
	case 3:	data = wd17xx_data_r(device, 0);
	}

	/* FD1791 has inverted data lines */
	return data ^ 0xff;
}

static WRITE8_DEVICE_HANDLER( slow_fdc_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	if (!conkort->wait_enable && !conkort->fdc_irq && !conkort->fdc_drq)
	{
		logerror("WAIT\n");
		/* TODO: this is really connected to the Z80 WAIT line */
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	/* FD1791 has inverted data lines */
	data ^= 0xff;

	switch (offset & 0x03)
	{
	case 0: wd17xx_command_w(device, 0, data); break;
	case 1:	wd17xx_track_w(device, 0, data);   break;
	case 2: wd17xx_sector_w(device, 0, data);  break;
	case 3: wd17xx_data_w(device, 0, data);    break;
	}
}

/* Fast Controller */

WRITE8_DEVICE_HANDLER( luxor_55_21046_cs_w )
{
	fast_t *conkort = get_safe_token_fast(device);

	conkort->cs = (data == input_port_read(device->machine, "SW3"));
}

READ8_DEVICE_HANDLER( luxor_55_21046_stat_r )
{
	fast_t *conkort = get_safe_token_fast(device);

	UINT8 data = 0xff;

	if (conkort->cs)
	{
		// TODO: D0 handling (NANDed with something)
		// LS240 inverts the data, D4/D5 not connected
		data = (~conkort->status & 0xcf) | 0x31;
	}

	return data;
}

READ8_DEVICE_HANDLER( luxor_55_21046_inp_r )
{
	fast_t *conkort = get_safe_token_fast(device);

	UINT8 data = 0xff;

	if (conkort->cs)
	{
		data = conkort->data_out;
	}

	return data;
}

WRITE8_DEVICE_HANDLER( luxor_55_21046_utp_w )
{
	fast_t *conkort = get_safe_token_fast(device);

	if (conkort->cs)
	{
		conkort->data_in = data;
	}
}

WRITE8_DEVICE_HANDLER( luxor_55_21046_c1_w )
{
	fast_t *conkort = get_safe_token_fast(device);

	if (conkort->cs)
	{
		cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, ASSERT_LINE);
		cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, CLEAR_LINE);
	}
}

WRITE8_DEVICE_HANDLER( luxor_55_21046_c3_w )
{
	fast_t *conkort = get_safe_token_fast(device);

	if (conkort->cs)
	{
		cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
	}
}

WRITE_LINE_DEVICE_HANDLER( luxor_55_21046_rst_w )
{
	if (!state)
	{
		device->reset();
	}
}

static READ8_DEVICE_HANDLER( fast_3d_r )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	return conkort->data_in;
}

static WRITE8_DEVICE_HANDLER( fast_4d_w )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	conkort->data_out = data;
}

static WRITE8_DEVICE_HANDLER( fast_4b_w )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	conkort->status = data;
}

static WRITE8_DEVICE_HANDLER( fast_9b_w )
{
	/*

		bit		description

		0		_MOTEA
		1		_DRVSB
		2		_DRVSA
		3		_MOTEB
		4		?
		5		_SIDE1
		6
		7

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	/* motor enable */
	floppy_mon_w(conkort->image0, !BIT(data, 0));
	floppy_mon_w(conkort->image1, !BIT(data, 3));
	floppy_drive_set_ready_state(conkort->image0, 1, 1);
	floppy_drive_set_ready_state(conkort->image1, 1, 1);

	/* drive select */
	if (BIT(data, 2)) wd17xx_set_drive(device, 0);
	if (BIT(data, 1)) wd17xx_set_drive(device, 1);

	/* side select */
	wd17xx_set_side(device, BIT(data, 5));
}

static WRITE8_DEVICE_HANDLER( fast_8a_w )
{
	/*

		bit		description

		0		FD1793 _MR
		1		FD1793 _DDEN, FDC9229 DENS
		2		FDC9229 MINI
		3		READY signal polarity (0=inverted)
		4		FDC9229 P2
		5		FDC9229 P1
		6
		7

		FDC9229 P0 is grounded

	*/

	/* master reset */
	wd17xx_mr_w(device, BIT(data, 0));

	/* density select */
	wd17xx_dden_w(device, BIT(data, 1));
}

/* Memory Maps */

// Slow Controller

static ADDRESS_MAP_START( slow_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_ROM AM_REGION("abc830:abc830", 0)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( slow_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80PIO_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0xb0, 0xb3) AM_MIRROR(0x0c) AM_DEVREADWRITE(FD1791_TAG, slow_fdc_r, slow_fdc_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_DEVWRITE(FD1791_TAG, slow_status_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_DEVWRITE(FD1791_TAG, slow_ctrl_w)
ADDRESS_MAP_END

// Fast Controller

static ADDRESS_MAP_START( fast_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM AM_REGION("abc830:conkort", 0)
	AM_RANGE(0x2000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fast_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x0f) AM_DEVREAD(SAB1793_TAG,  fast_3d_r)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0f) AM_DEVWRITE(SAB1793_TAG, fast_4d_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x0f) AM_DEVWRITE(SAB1793_TAG, fast_4b_w)
	AM_RANGE(0x30, 0x30) AM_MIRROR(0x0f) AM_DEVWRITE(SAB1793_TAG, fast_9b_w)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x0f) AM_DEVWRITE(SAB1793_TAG, fast_8a_w)
	AM_RANGE(0x50, 0x50) AM_MIRROR(0x0f) AM_READ_PORT("SW1")
	AM_RANGE(0x60, 0x63) AM_MIRROR(0x0c) AM_DEVREAD(SAB1793_TAG, wd17xx_r)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0x0c) AM_DEVWRITE(SAB1793_TAG, wd17xx_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) AM_DEVREADWRITE(Z80DMA_TAG, z80dma_r, z80dma_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( luxor_55_21046 )
	PORT_START("abc830:SW1")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPNAME( 0x05, 0x00, "Drive 0" ) PORT_DIPLOCATION("SW1:1,3") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, "SS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x04, "SS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x01, "DS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x05, "DS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x0a, 0x00, "Drive 1" ) PORT_DIPLOCATION("SW1:2,4") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, "SS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x08, "SS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x02, "DS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x0a, "DS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x05, 0x01, "Drive 0" ) PORT_DIPLOCATION("SW1:1,3") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "SS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x04, "SS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x01, "DS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x05, "DS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0x0a, 0x02, "Drive 1" ) PORT_DIPLOCATION("SW1:2,4") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "SS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x08, "SS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x02, "DS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x0a, "DS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0xf0, 0x80, "Disk Drive" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x80, "BASF 6106/08 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x90, "MPI 51 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x40, "BASF 6118 (ABC 832, 190 9711-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x30, "Micropolis 1015F (ABC 832, 190 9711-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x50, "Micropolis 1115F (ABC 832, 190 9711-17)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x10, "TEAC FD55F (ABC 834, 230 7802-01)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x20, "BASF 6138 (ABC 850, 230 8440-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0xe0, "BASF 6105" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPSETTING(    0xf0, "BASF 6106 (ABC 838, 230 8838-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)

	PORT_START("SW3")
	PORT_DIPNAME( 0x7f, 0x2d, "Disk Drive" ) PORT_DIPLOCATION("SW3:1,2,3,4,5,6,7")
	PORT_DIPSETTING(    0x2c, "ABC 832/834/850" )
	PORT_DIPSETTING(    0x2d, "ABC 830" )
	PORT_DIPSETTING(    0x2e, "ABC 838" )

	PORT_START("CONKORT_ROM")
	PORT_CONFNAME( 0x07, 0x00, "Controller PROM" )
	PORT_CONFSETTING(    0x00, "Luxor v1.07" )
	PORT_CONFSETTING(    0x01, "Luxor v1.08" )
	PORT_CONFSETTING(    0x02, "DIAB v2.07" )
INPUT_PORTS_END

/* Z80 PIO */

static READ8_DEVICE_HANDLER( conkort_pio_port_a_r )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	return conkort->data;
}

static WRITE8_DEVICE_HANDLER( conkort_pio_port_a_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->data = data;
}

static READ8_DEVICE_HANDLER( conkort_pio_port_b_r )
{
	/*

        bit     description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head load
        7       FDC IRQ         interrupt request

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	UINT8 data = 4;

	/* single/double sided drive */
//  data |= 0x01;

	/* single/double density drive */
	data |= 0x02;

	/* radial/binary drive select */
	data |= 0x10;

	/* head load */
//  data |= wd17xx_hdld_r(device) << 6;
	data |= 0x40;

	/* FDC interrupt request */
	data |= conkort->fdc_irq << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( conkort_pio_port_b_w )
{
	/*

        bit     signal          description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head load
        7       FDC IRQ         interrupt request

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	/* double density enable */
	wd17xx_dden_w(conkort->fd1791, BIT(data, 3));

	/* head load timing */
//  wd17xx_hlt_w(conkort->fd1791, BIT(data, 5));
}

static Z80PIO_INTERFACE( conkort_pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),		/* interrupt callback */
	DEVCB_HANDLER(conkort_pio_port_a_r),	/* port A read callback */
	DEVCB_HANDLER(conkort_pio_port_a_w),	/* port A write callback */
	DEVCB_NULL,								/* port A ready callback */
	DEVCB_HANDLER(conkort_pio_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(conkort_pio_port_b_w),	/* port B write callback */
	DEVCB_NULL								/* port B ready callback */
};

static const z80_daisy_config slow_daisy_chain[] =
{
	{ Z80PIO_TAG },
	{ NULL }
};

/* Z80 DMA */

/*

    DMA Transfer Programs

    READ DAM
    --------
    7D 45 21 05 00 C3 14 28 95 6B 02 8A CF 01 AF CF 87

    7D  transfer mode, port A -> port B, port A starting address follows, block length follows
    45  port A starting address low byte = 45
    21  port A starting address high byte = 21
    05  block length low byte = 05
    00  block length high byte = 00
    C3  reset
    14  port A is memory, port A address increments
    28  port B is I/O, port B address fixed
    95  byte mode, port B starting address low byte follows, interrupt control byte follows
    6B  port B starting address low byte = 6B (FDC DATA read)
    02  interrupt at end of block
    8A  ready active high
    CF  load
    01  transfer B->A
    AF  disable interrupts
    CF  load
    87  enable DMA

    ??
    --
    C3 14 28 95 7B 02 8A CF 05 AF CF 87

    C3  reset
    14  port A is memory, port A address increments
    28  port B is I/O, port B address fixed
    95  byte mode, port B starting address low follows, interrupt control byte follows
    7B  port B starting address 0x007B (FDC DATA write)
    02  interrupt at end of block
    8A  ready active high
    CF  load
    05  transfer A->B
    AF  disable interrupts
    CF  load
    87  enable DMA

    ??
    --
    C3 91 40 8A AB

    C3  reset
    91  byte mode, interrupt control byte follows
    40  interrupt on RDY
    8A  _CE only, ready active high, stop on end of block
    AB  enable interrupts

*/

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( dma_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_HALT),
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_write_byte)
};

static const z80_daisy_config fast_daisy_chain[] =
{
	{ Z80DMA_TAG },
	{ NULL }
};

/* FD1791 */

static WRITE_LINE_DEVICE_HANDLER( slow_fd1791_intrq_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->fdc_irq = state;
	z80pio_pb_w(conkort->z80pio, 0, state << 7);

	if (state)
	{
		/* TODO: this is really connected to the Z80 WAIT line */
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static WRITE_LINE_DEVICE_HANDLER( slow_fd1791_drq_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->fdc_drq = state;

	if (state)
	{
		/* TODO: this is really connected to the Z80 WAIT line */
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static const wd17xx_interface slow_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(slow_fd1791_intrq_w),
	DEVCB_LINE(slow_fd1791_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* FD1793 */

static WRITE_LINE_DEVICE_HANDLER( fast_fd1793_intrq_w )
{
	fast_t *conkort = get_safe_token_fast(device->owner());

	cpu_set_input_line(conkort->cpu, INPUT_LINE_IRQ0, state);
}

static const wd17xx_interface fast_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(fast_fd1793_intrq_w),
	DEVCB_DEVICE_LINE(Z80DMA_TAG, z80dma_rdy_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static const floppy_config abc800_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD, // FIXME
	FLOPPY_OPTIONS_NAME(abc80),
	NULL
};

/* Machine Driver */

static MACHINE_CONFIG_FRAGMENT( luxor_55_10828 )
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz/2)
	MDRV_CPU_PROGRAM_MAP(slow_map)
	MDRV_CPU_IO_MAP(slow_io_map)
	MDRV_CPU_CONFIG(slow_daisy_chain)

	MDRV_Z80PIO_ADD(Z80PIO_TAG, XTAL_4MHz/2, conkort_pio_intf)
	MDRV_WD179X_ADD(FD1791_TAG, slow_wd17xx_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(abc800_floppy_config)
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( luxor_55_21046 )
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
	MDRV_CPU_PROGRAM_MAP(fast_map)
	MDRV_CPU_IO_MAP(fast_io_map)
	MDRV_CPU_CONFIG(fast_daisy_chain)

	MDRV_Z80DMA_ADD(Z80DMA_TAG, XTAL_16MHz/4, dma_intf)
	MDRV_WD1793_ADD(SAB1793_TAG, fast_wd17xx_interface)

	MDRV_FLOPPY_2_DRIVES_ADD(abc800_floppy_config)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( luxor_55_10828 )
	ROM_REGION( 0x1000, "abc830", ROMREGION_LOADBYNAME )
	ROM_LOAD( "mpi02.bin",    0x0000, 0x0800, CRC(2aac9296) SHA1(c01a62e7933186bdf7068d2e9a5bc36590544349) ) // MPI 51 (5510760-01)
	ROM_LOAD( "basf6106.bin", 0x0800, 0x0800, NO_DUMP ) // BASF 6106

	ROM_REGION( 0x1800, "abc832", ROMREGION_LOADBYNAME )
	ROM_LOAD( "micr1015.bin", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015
	ROM_LOAD( "micr1115.bin", 0x0800, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115 (v2.3)
	ROM_LOAD( "basf6118.bin", 0x1000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118 (v1.2)

	ROM_REGION( 0x1000, "abc838", ROMREGION_LOADBYNAME )
	ROM_LOAD( "basf6104.bin", 0x0800, 0x0800, NO_DUMP ) // BASF 6104
	ROM_LOAD( "basf6115.bin", 0x0800, 0x0800, NO_DUMP ) // BASF 6115
ROM_END

ROM_START( luxor_55_21046 )
	ROM_REGION( 0x10000, "conkort", ROMREGION_LOADBYNAME )
	ROM_LOAD( "fast108.6cd", 0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.6cd", 0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07
	ROM_LOAD( "cntr 1.07 6490318-07.6cd", 0x0000, 0x2000, CRC(06ae1fe8) SHA1(ad1d9d0c192539af70cb95223263915a09693ef8) ) // PROM v1.07, Art N/O 6490318-07. Luxor Styrkort Art. N/O 55 21046-41. Date 1985-07-03
ROM_END

ROM_START( myab_turbo_kontroller )
	ROM_REGION( 0x3000, "myab", ROMREGION_LOADBYNAME )
	ROM_LOAD( "unidis5d.bin", 0x0000, 0x1000, CRC(569dd60c) SHA1(47b810bcb5a063ffb3034fd7138dc5e15d243676) ) // 5" 25-pin
	ROM_LOAD( "unidiskh.bin", 0x1000, 0x1000, CRC(5079ad85) SHA1(42bb91318f13929c3a440de3fa1f0491a0b90863) ) // 5" 34-pin
	ROM_LOAD( "unidisk8.bin", 0x2000, 0x1000, CRC(d04e6a43) SHA1(8db504d46ff0355c72bd58fd536abeb17425c532) ) // 8"
ROM_END

ROM_START( hdd_controller )
	ROM_REGION( 0x1000, "abc850", ROMREGION_LOADBYNAME )
	ROM_LOAD( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime RO202 (http://artofhacking.com/th99/h/txt/3699.txt)
	ROM_LOAD( "basf6185.bin", 0x0800, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185 (http://bk0010.narod.ru/hardware_specs/h/txt/178.txt)

	ROM_REGION( 0x1000, "abc852", ROMREGION_LOADBYNAME )
	ROM_LOAD( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126

	ROM_REGION( 0x800, "abc856", ROMREGION_LOADBYNAME )
	ROM_LOAD( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325

	ROM_REGION( 0x1000, "xebec", ROMREGION_LOADBYNAME )
	ROM_LOAD( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038
	ROM_LOAD( "st225.bin",    0x0800, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225
ROM_END

/*-------------------------------------------------
    DEVICE_START( luxor_55_10828 )
-------------------------------------------------*/

static DEVICE_START( luxor_55_10828 )
{
	slow_t *conkort = get_safe_token_slow(device);

	/* find our CPU */
	conkort->cpu = device->subdevice(Z80_TAG);

	/* find devices */
	conkort->z80pio = device->subdevice(Z80PIO_TAG);
	conkort->fd1791 = device->subdevice(FD1791_TAG);
	conkort->image0 = device->subdevice(FLOPPY_0);
	conkort->image1 = device->subdevice(FLOPPY_1);

	/* register for state saving */
	state_save_register_device_item(device, 0, conkort->cs);
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
	state_save_register_device_item(device, 0, conkort->fdc_drq);
	state_save_register_device_item(device, 0, conkort->wait_enable);

	/* patch out protection checks */
	UINT8 *rom = device->subregion("abc830")->base();
	rom[0x0718] = 0xff;
	rom[0x072c] = 0xff;
	rom[0x0336] = 0xff;
	rom[0x00fa] = 0xff;
	rom[0x0771] = 0xff;
	rom[0x0788] = 0xff;
}

/*-------------------------------------------------
    DEVICE_RESET( luxor_55_10828 )
-------------------------------------------------*/

static DEVICE_RESET( luxor_55_10828 )
{
	slow_t *conkort = get_safe_token_slow(device);

	conkort->cpu->reset();

	conkort->cs = 0;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( luxor_55_10828 )
-------------------------------------------------*/

DEVICE_GET_INFO( luxor_55_10828 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(slow_t);									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(luxor_55_10828);					break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(luxor_55_10828);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_10828);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(luxor_55_10828);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor 55 10828-01");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_START( luxor_55_21046 )
-------------------------------------------------*/

static DEVICE_START( luxor_55_21046 )
{
	fast_t *conkort = get_safe_token_fast(device);

	/* find our CPU */
	conkort->cpu = device->subdevice(Z80_TAG);

	/* find devices */
	conkort->z80dma = device->subdevice(Z80DMA_TAG);
	conkort->wd1793 = device->subdevice(SAB1793_TAG);
	conkort->image0 = device->subdevice(FLOPPY_0);
	conkort->image1 = device->subdevice(FLOPPY_1);

	/* register for state saving */
	state_save_register_device_item(device, 0, conkort->cs);
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data_in);
	state_save_register_device_item(device, 0, conkort->data_out);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
}

/*-------------------------------------------------
    DEVICE_RESET( luxor_55_21046 )
-------------------------------------------------*/

static DEVICE_RESET( luxor_55_21046 )
{
	fast_t *conkort = get_safe_token_fast(device);

	conkort->cpu->reset();

	conkort->cs = 0;

	floppy_mon_w(conkort->image0, ASSERT_LINE);
	floppy_mon_w(conkort->image1, ASSERT_LINE);
	floppy_drive_set_ready_state(conkort->image0, 1, 1);
	floppy_drive_set_ready_state(conkort->image1, 1, 1);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( luxor_55_21046 )
-------------------------------------------------*/

DEVICE_GET_INFO( luxor_55_21046 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(fast_t);									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(luxor_55_21046);					break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(luxor_55_21046);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_21046);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(luxor_55_21046);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor Conkort 55 21046-xx");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

FLOPPY_OPTIONS_START(abc80)
	FLOPPY_OPTION(abc80, "dsk", "Scandia Metric FD2", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 830", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 832/834", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(abc80, "dsk", "ABC 838", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

DEFINE_LEGACY_DEVICE(LUXOR_55_10828, luxor_55_10828);
DEFINE_LEGACY_DEVICE(LUXOR_55_21046, luxor_55_21046);
