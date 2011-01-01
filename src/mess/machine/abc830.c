/*

Luxor Conkort

PCB Layout
----------

55 10900-01

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
|   S1                              |
|   LS125   LS124     S20   DM8131  |
|                                   |
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM     - Hitachi HN462716 2Kx8 EPROM "MPI02"
    Z80     - Sharp LH0080A Z80A CPU
    Z80PIO  - SGS Z8420AB1 Z80A PIO
    MB8876  - Mitsubishi MB8876 Floppy Disc Controller (FD1791 compatible)
    TC5514  - Toshiba TC5514AP-2 1Kx4 bit Static RAM
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    C140E   - Ferranti 2C140E "copy protection device"
    N8T97N  - SA N8T97N ?
    CON1    - ABC bus connector
    CON2    - 25-pin D sub floppy connector (AMP4284)
    SW1     - Disk drive type (SS/DS, SD/DD)
    S1      - ABC bus card address bit 0
    S2      -
    S3      -
    S4      -
    S5      -
    S6      -
    S7      -
    S8      -
    S9      -
    LD1     - LED

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

    - S2-S5 jumpers
	- ABC80 ERR 48 on boot
	- side select makes controller go crazy and try to WRITE_TRK

*/

#include "emu.h"
#include "abc830.h"
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
	int sel0;				/* drive select 0 */
	int sel1;				/* drive select 1 */

	/* devices */
	device_t *bus;
	device_t *cpu;
	device_t *z80pio;
	device_t *fd1791;
	device_t *image0;
	device_t *image1;
};

typedef struct _fast_t fast_t;
struct _fast_t
{
	int cs;					/* card selected */
	UINT8 status;			/* ABC BUS status */
	UINT8 data_in;			/* ABC BUS data in */
	UINT8 data_out;			/* ABC BUS data out */
	int fdc_irq;			/* FDC interrupt */
	int dma_irq;			/* DMA interrupt */
	int busy;				/* busy bit */
	int force_busy;			/* force busy bit */
	UINT8 sw1;				/* DS/DD */
	UINT8 sw2;				/* drive type */
	UINT8 sw3;				/* ABC bus address */

	/* devices */
	device_t *bus;
	device_t *cpu;
	device_t *z80dma;
	device_t *wd1793;
	device_t *image0;
	device_t *image1;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE slow_t *get_safe_token_slow(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == ABC830_PIO);
	return (slow_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE fast_t *get_safe_token_fast(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == ABC830) || (device->type() == ABC832) || (device->type() == ABC838));
	return (fast_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE conkort_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == ABC830_PIO) || (device->type() == ABC830) || (device->type() == ABC832) || (device->type() == ABC838));
	return (conkort_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* Slow Controller */

WRITE8_DEVICE_HANDLER( luxor_55_10828_cs_w )
{
	slow_t *conkort = get_safe_token_slow(device);
	UINT8 address = 0x2c | BIT(input_port_read(device->machine, "luxor_55_10828:S1"), 0);

	conkort->cs = (data == address);
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

	// drive selection
	if (BIT(data, 0)) wd17xx_set_drive(device, 0);
	if (BIT(data, 1)) wd17xx_set_drive(device, 1);
//  if (BIT(data, 2)) wd17xx_set_drive(device, 2);
	conkort->sel0 = BIT(data, 0);
	conkort->sel1 = BIT(data, 1);

	// motor enable
	int mtron = BIT(data, 3);
	floppy_mon_w(conkort->image0, mtron);
	floppy_mon_w(conkort->image1, mtron);
	floppy_drive_set_ready_state(conkort->image0, !mtron, 1);
	floppy_drive_set_ready_state(conkort->image1, !mtron, 1);

	// side selection
	//wd17xx_set_side(device, BIT(data, 4));

	// wait enable
	conkort->wait_enable = BIT(data, 6);

	// FDC master reset
	wd17xx_mr_w(device, BIT(data, 7));
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

	// interrupt
//	abcbus_int_w(conkort->bus, BIT(data, 0) ? CLEAR_LINE : ASSERT_LINE);

	conkort->status = data & 0xfe;
}

static READ8_DEVICE_HANDLER( slow_fdc_r )
{
	slow_t *conkort = get_safe_token_slow(device->owner());
	UINT8 data = 0xff;

	if (!conkort->wait_enable && !conkort->fdc_irq && !conkort->fdc_drq)
	{
		// TODO: this is really connected to the Z80 WAIT line
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	switch (offset & 0x03)
	{
	case 0: data = wd17xx_status_r(device, 0); break;
	case 1: data = wd17xx_track_r(device, 0); break;
	case 2: data = wd17xx_sector_r(device, 0); break;
	case 3: data = wd17xx_data_r(device, 0); break;
	}

	// FD1791 has inverted data lines
	return data ^ 0xff;
}

static WRITE8_DEVICE_HANDLER( slow_fdc_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	if (!conkort->wait_enable && !conkort->fdc_irq && !conkort->fdc_drq)
	{
		logerror("WAIT\n");
		// TODO: this is really connected to the Z80 WAIT line
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, ASSERT_LINE);
	}

	// FD1791 has inverted data lines
	data ^= 0xff;

	switch (offset & 0x03)
	{
	case 0: wd17xx_command_w(device, 0, data); break;
	case 1: wd17xx_track_w(device, 0, data); break;
	case 2: wd17xx_sector_w(device, 0, data); break;
	case 3: wd17xx_data_w(device, 0, data); break;
	}
}

/* Fast Controller */

WRITE8_DEVICE_HANDLER( luxor_55_21046_cs_w )
{
	fast_t *conkort = get_safe_token_fast(device);

//	conkort->cs = (data == input_port_read(device->machine, "luxor_55_21046:SW3"));
	conkort->cs = (data == conkort->sw3);
}

READ8_DEVICE_HANDLER( luxor_55_21046_stat_r )
{
	fast_t *conkort = get_safe_token_fast(device);

	UINT8 data = 0;

	if (conkort->cs)
	{
		data = (conkort->status & 0xce) | conkort->busy;
	}

	// LS240 inverts the data
	return data ^ 0xff;
}

READ8_DEVICE_HANDLER( luxor_55_21046_inp_r )
{
	fast_t *conkort = get_safe_token_fast(device);

	UINT8 data = 0xff;

	if (conkort->cs)
	{
		data = conkort->data_out;
		conkort->busy = 1;
	}

	return data;
}

WRITE8_DEVICE_HANDLER( luxor_55_21046_utp_w )
{
	fast_t *conkort = get_safe_token_fast(device);

	if (conkort->cs)
	{
		conkort->data_in = data;
		conkort->busy = 1;
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
		cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, ASSERT_LINE);
		cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, CLEAR_LINE);
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
	fast_t *conkort = get_safe_token_fast(device->owner());

	if (BIT(conkort->status, 0))
	{
		conkort->busy = 0;
	}

	return conkort->data_in;
}

static WRITE8_DEVICE_HANDLER( fast_4d_w )
{
	fast_t *conkort = get_safe_token_fast(device->owner());
	
	if (BIT(conkort->status, 0))
	{
		conkort->busy = 0;
	}

	conkort->data_out = data;
}

static WRITE8_DEVICE_HANDLER( fast_4b_w )
{
	/*

		bit		description

		0		force busy
		1		
		2		
		3		
		4		N/C
		5		N/C
		6		
		7		

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	conkort->status = data;

	/* busy */
	if (!BIT(conkort->status, 0))
	{
		conkort->busy = 1;
	}
}

static WRITE8_DEVICE_HANDLER( fast_9b_w )
{
	/*

		bit		signal		description

		0		DS0			drive select 0
		1		DS1			drive select 1
		2		DS2			drive select 2
		3		MTRON		motor on
		4		TG43		track > 43
		5		SIDE1		side 1 select
		6
		7

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	// drive select
	if (BIT(data, 0)) wd17xx_set_drive(device, 0);
	if (BIT(data, 1)) wd17xx_set_drive(device, 1);
	//if (BIT(data, 2)) wd17xx_set_drive(device, 2);

	// motor enable
	int mtron = BIT(data, 3);
	floppy_mon_w(conkort->image0, !mtron);
	floppy_mon_w(conkort->image1, !mtron);
	floppy_drive_set_ready_state(conkort->image0, mtron, 1);
	floppy_drive_set_ready_state(conkort->image1, mtron, 1);

	// side select
	wd17xx_set_side(device, BIT(data, 5));
}

static WRITE8_DEVICE_HANDLER( fast_8a_w )
{
	/*

		bit		signal							description

		0		FD1793 _MR						FDC master reset
		1		FD1793 _DDEN, FDC9229 DENS		density select
		2		FDC9229 MINI					
		3		READY signal polarity			(0=inverted)
		4		FDC9229 P2						
		5		FDC9229 P1						
		6
		7

		FDC9229 P0 is grounded

	*/

	// FDC master reset
	wd17xx_mr_w(device, BIT(data, 0));

	// density select
	wd17xx_dden_w(device, BIT(data, 1));
}

static READ8_DEVICE_HANDLER( fast_9a_r )
{
	/*

		bit		signal		description

		0		busy		controller busy
		1		_FD2S		double-sided disk
		2		SW2			
		3		_DCG ?		disk changed
		4		SW1-1
		5		SW1-2
		6		SW1-3
		7		SW1-4

	*/

	fast_t *conkort = get_safe_token_fast(device->owner());

	UINT8 data = 0;

	// busy
	data |= conkort->busy;

	// SW1
//	UINT8 sw1 = input_port_read(device->machine, "luxor_55_21046:SW1") & 0x0f;
	UINT8 sw1 = conkort->sw1;

	data |= sw1 << 4;

	// SW2
//	UINT8 sw2 = input_port_read(device->machine, "luxor_55_21046:SW2") & 0x0f;
	UINT8 sw2 = conkort->sw2;
	
	// TTL inputs float high so DIP switch in off position equals 1
	int sw2_1 = BIT(sw2, 0) ? 1 : BIT(offset, 8);
	int sw2_2 = BIT(sw2, 1) ? 1 : BIT(offset, 9);
	int sw2_3 = BIT(sw2, 2) ? 1 : BIT(offset, 10);
	int sw2_4 = BIT(sw2, 3) ? 1 : BIT(offset, 11);
	int sw2_data = !(sw2_1 & sw2_2 & !(sw2_3 ^ sw2_4));
	
	data |= sw2_data << 2;

	return data ^ 0xff;
}

/* Memory Maps */

// Slow Controller

static ADDRESS_MAP_START( slow_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_ROM AM_REGION("luxor_55_10828:abc830", 0)
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
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x1fff) AM_ROM AM_REGION("luxor_55_21046:conkort", 0x2000)
	AM_RANGE(0x2000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fast_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff0f) AM_DEVREAD(SAB1793_TAG, fast_3d_r)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0xff0f) AM_DEVWRITE(SAB1793_TAG, fast_4d_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0xff0f) AM_DEVWRITE(SAB1793_TAG, fast_4b_w)
	AM_RANGE(0x30, 0x30) AM_MIRROR(0xff0f) AM_DEVWRITE(SAB1793_TAG, fast_9b_w)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0xff0f) AM_DEVWRITE(SAB1793_TAG, fast_8a_w)
	AM_RANGE(0x50, 0x50) AM_MIRROR(0xff0f) AM_MASK(0xff00) AM_DEVREAD(SAB1793_TAG, fast_9a_r)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0xff0c) AM_DEVREAD(SAB1793_TAG, wd17xx_r)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0xff0c) AM_DEVWRITE(SAB1793_TAG, wd17xx_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0xff0f) AM_DEVREADWRITE(Z80DMA_TAG, z80dma_r, z80dma_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( luxor_55_10828 )
	PORT_START("luxor_55_10828:SW1")
	PORT_DIPNAME( 0x01, 0x01, "Drive 0 Sided" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x02, 0x02, "Drive 1 Sided" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x04, 0x00, "Drive 0 Density" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )
	PORT_DIPNAME( 0x08, 0x00, "Drive 1 Density" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, "Double" )

	PORT_START("luxor_55_10828:S1")
	PORT_DIPNAME( 0x01, 0x01, "Card Address" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(    0x00, "44 (ABC 832/834/850)" )
	PORT_DIPSETTING(    0x01, "45 (ABC 830)" )
INPUT_PORTS_END

INPUT_PORTS_START( luxor_55_21046 )
	PORT_START("luxor_55_21046:SW1")
	// ABC 838
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW1:1,2,3,4") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPSETTING(    0x00, DEF_STR( Unused ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	// ABC 830
	PORT_DIPNAME( 0x01, 0x00, "Drive 0 Sided" ) PORT_DIPLOCATION("SW1:1") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x01, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x02, 0x00, "Drive 1 Sided" ) PORT_DIPLOCATION("SW1:2") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x02, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x04, 0x00, "Drive 0 Density" ) PORT_DIPLOCATION("SW1:3") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x04, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x08, 0x00, "Drive 1 Density" ) PORT_DIPLOCATION("SW1:4") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x08, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	// ABC 832/834/850
	PORT_DIPNAME( 0x01, 0x01, "Drive 0 Sided" ) PORT_DIPLOCATION("SW1:1") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x01, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0x02, 0x02, "Drive 1 Sided" ) PORT_DIPLOCATION("SW1:2") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, DEF_STR( Single ) ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x02, "Double" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0x04, 0x00, "Drive 0 Tracks" ) PORT_DIPLOCATION("SW1:3") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "80" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x04, "40" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0x08, 0x00, "Drive 1 Tracks" ) PORT_DIPLOCATION("SW1:4") PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "80" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x08, "40" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c)

	PORT_START("luxor_55_21046:SW2")
	PORT_DIPNAME( 0x0f, 0x01, "Drive Type" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x01, "TEAC FD55F (ABC 834)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c) // 230 7802-01
	PORT_DIPSETTING(    0x02, "BASF 6138 (ABC 850)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c) // 230 8440-15
	PORT_DIPSETTING(    0x03, "Micropolis 1015F (ABC 832)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c) // 190 9711-15
	PORT_DIPSETTING(    0x04, "BASF 6118 (ABC 832)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c) // 190 9711-16
	PORT_DIPSETTING(    0x05, "Micropolis 1115F (ABC 832)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2c) // 190 9711-17
	PORT_DIPSETTING(    0x08, "BASF 6106/08 (ABC 830)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d) // 190 9206-16
	PORT_DIPSETTING(    0x09, "MPI 51 (ABC 830)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2d) // 190 9206-16
	PORT_DIPSETTING(    0x0e, "BASF 6105 (ABC 838)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPSETTING(    0x0f, "BASF 6106 (ABC 838)" ) PORT_CONDITION("luxor_55_21046:SW3", 0x7f, PORTCOND_EQUALS, 0x2e) // 230 8838-15
	
	PORT_START("luxor_55_21046:SW3")
	PORT_DIPNAME( 0x7f, 0x2c, "Card Address" ) PORT_DIPLOCATION("SW3:1,2,3,4,5,6,7")
	PORT_DIPSETTING(    0x2c, "44 (ABC 832/834/850)" )
	PORT_DIPSETTING(    0x2d, "45 (ABC 830)" )
	PORT_DIPSETTING(    0x2e, "46 (ABC 838)" )

	PORT_START("luxor_55_21046:S6")
	PORT_DIPNAME( 0x01, 0x01, "RAM Size" ) PORT_DIPLOCATION("S6:1")
	PORT_DIPSETTING(    0x00, "2 KB" )
	PORT_DIPSETTING(    0x01, "8 KB" )

	PORT_START("luxor_55_21046:S8")
	PORT_DIPNAME( 0x01, 0x01, "Drive Type" ) PORT_DIPLOCATION("S8:1")
	PORT_DIPSETTING(    0x00, "8\"" )
	PORT_DIPSETTING(    0x01, "5.25\"" )

	PORT_START("luxor_55_21046:S9")
	PORT_DIPNAME( 0x01, 0x01, "RDY Pin" ) PORT_DIPLOCATION("S9:1")
	PORT_DIPSETTING(    0x00, "P2-6 (8\")" )
	PORT_DIPSETTING(    0x01, "P2-34 (5.25\")" )
INPUT_PORTS_END

/* Z80 PIO */

static READ8_DEVICE_HANDLER( pio_pa_r )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	return conkort->data;
}

static WRITE8_DEVICE_HANDLER( pio_pa_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->data = data;
}

static READ8_DEVICE_HANDLER( pio_pb_r )
{
	/*

        bit     description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head loaded
        7       FDC IRQ         interrupt request

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	UINT8 data = 0x04;

	// single/double sided drive
	UINT8 sw1 = input_port_read(device->machine, "luxor_55_10828:SW1") & 0x0f;
	int ds0 = conkort->sel0 ? BIT(sw1, 0) : 1;
	int ds1 = conkort->sel1 ? BIT(sw1, 1) : 1;
	data |= !(ds0 & ds1);

	// single/double density drive
	int dd0 = conkort->sel0 ? BIT(sw1, 2) : 1;
	int dd1 = conkort->sel1 ? BIT(sw1, 3) : 1;
	data |= !(dd0 & dd1) << 1;

	// radial/binary drive select
	data |= 0x10;

	// head load
//  data |= wd17xx_hdld_r(device) << 6;
	data |= 0x40;

	// FDC interrupt request
	data |= conkort->fdc_irq << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( pio_pb_w )
{
	/*

        bit     signal          description

        0       !(_DS0 & _DS1)  single/double sided (0=SS, 1=DS)
        1       !(_DD0 & _DD1)  single/double density (0=DS, 1=DD)
        2       8B pin 10
        3       FDC _DDEN       double density enable
        4       _R/BS           radial/binary drive select
        5       FDC HLT         head load timing
        6       FDC _HDLD       head loaded
        7       FDC IRQ         interrupt request

    */

	slow_t *conkort = get_safe_token_slow(device->owner());

	// double density enable
	wd17xx_dden_w(conkort->fd1791, BIT(data, 3));

	// head load timing
//  wd17xx_hlt_w(conkort->fd1791, BIT(data, 5));
}

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt callback */
	DEVCB_HANDLER(pio_pa_r),						/* port A read callback */
	DEVCB_HANDLER(pio_pa_w),						/* port A write callback */
	DEVCB_NULL,										/* port A ready callback */
	DEVCB_HANDLER(pio_pb_r),						/* port B read callback */
	DEVCB_HANDLER(pio_pb_w),						/* port B write callback */
	DEVCB_NULL										/* port B ready callback */
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

    WRITE TO DISK
    -------------
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

static WRITE_LINE_DEVICE_HANDLER( z80dma_int_w )
{
	fast_t *conkort = get_safe_token_fast(device->owner());

	conkort->dma_irq = state;

	// FDC and DMA interrupts are wire-ORed to the Z80
	cpu_set_input_line(conkort->cpu, INPUT_LINE_IRQ0, conkort->fdc_irq | conkort->dma_irq);
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( dma_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_HALT),
	DEVCB_LINE(z80dma_int_w),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_write_byte)
};

/* FD1791 */

static WRITE_LINE_DEVICE_HANDLER( slow_fd1791_intrq_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->fdc_irq = state;
	z80pio_pb_w(conkort->z80pio, 0, state << 7);

	if (state)
	{
		// TODO: this is really connected to the Z80 WAIT line
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static WRITE_LINE_DEVICE_HANDLER( slow_fd1791_drq_w )
{
	slow_t *conkort = get_safe_token_slow(device->owner());

	conkort->fdc_drq = state;

	if (state)
	{
		// TODO: this is really connected to the Z80 WAIT line
		cpu_set_input_line(conkort->cpu, INPUT_LINE_HALT, CLEAR_LINE);
	}
}

static const wd17xx_interface slow_fdc_intf =
{
	DEVCB_NULL,
	DEVCB_LINE(slow_fd1791_intrq_w),
	DEVCB_LINE(slow_fd1791_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static FLOPPY_OPTIONS_START( fd2 )
	// NOTE: FD2 cannot be used with the Luxor controller card,
	// it has a proprietary one. This is just for reference.
	FLOPPY_OPTION(fd2, "dsk", "Scandia Metric FD2", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config fd2_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSSD,
    FLOPPY_OPTIONS_NAME(fd2),
    NULL
};

static FLOPPY_OPTIONS_START( abc830 )
	// NOTE: Real ABC 830 (160KB) disks use a 7:1 sector interleave.
	//
	// The controller ROM is patched to remove the interleaving so
	// you can use disk images with logical sector layout instead.
	//
	// Specify INTERLEAVE([7]) below if you prefer the physical layout.
	FLOPPY_OPTION(abc830, "dsk", "Luxor ABC 830", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config abc830_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSDD_40,
    FLOPPY_OPTIONS_NAME(abc830),
    NULL
};

static FLOPPY_OPTIONS_START( abc832 )
	FLOPPY_OPTION(abc832, "dsk", "Luxor ABC 832/834", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config abc832_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSQD,
    FLOPPY_OPTIONS_NAME(abc832),
    NULL
};

static FLOPPY_OPTIONS_START( abc838 )
	FLOPPY_OPTION(abc838, "dsk", "Luxor ABC 838", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config abc838_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_8_DSDD,
    FLOPPY_OPTIONS_NAME(abc838),
    NULL
};

/* FD1793 */

static WRITE_LINE_DEVICE_HANDLER( fast_fd1793_intrq_w )
{
	fast_t *conkort = get_safe_token_fast(device->owner());

	conkort->fdc_irq = state;

	// FDC and DMA interrupts are wire-ORed to the Z80
	cpu_set_input_line(conkort->cpu, INPUT_LINE_IRQ0, conkort->fdc_irq | conkort->dma_irq);
}

static const wd17xx_interface fast_fdc_intf =
{
	DEVCB_NULL,
	DEVCB_LINE(fast_fd1793_intrq_w),
	DEVCB_DEVICE_LINE(Z80DMA_TAG, z80dma_rdy_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* Machine Driver */

static MACHINE_CONFIG_FRAGMENT( luxor_55_10828 )
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz/2)
	MCFG_CPU_PROGRAM_MAP(slow_map)
	MCFG_CPU_IO_MAP(slow_io_map)
	MCFG_CPU_CONFIG(slow_daisy_chain)

	MCFG_Z80PIO_ADD(Z80PIO_TAG, XTAL_4MHz/2, pio_intf)
	MCFG_WD179X_ADD(FD1791_TAG, slow_fdc_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( abc830_pio, luxor_55_10828 )
	MCFG_FLOPPY_2_DRIVES_ADD(abc830_floppy_config)
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( luxor_55_21046 )
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
	MCFG_CPU_PROGRAM_MAP(fast_map)
	MCFG_CPU_IO_MAP(fast_io_map)

	MCFG_Z80DMA_ADD(Z80DMA_TAG, XTAL_16MHz/4, dma_intf)
	MCFG_WD1793_ADD(SAB1793_TAG, fast_fdc_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( abc830, luxor_55_21046 )
	MCFG_FLOPPY_2_DRIVES_ADD(abc830_floppy_config)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( abc832, luxor_55_21046 )
	MCFG_FLOPPY_2_DRIVES_ADD(abc832_floppy_config)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( abc838, luxor_55_21046 )
	MCFG_FLOPPY_2_DRIVES_ADD(abc838_floppy_config)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( luxor_55_10828 )
	ROM_REGION( 0x1000, "abc830", ROMREGION_LOADBYNAME )
	// MyAB Turbo-Kontroller
	ROM_LOAD_OPTIONAL( "unidis5d.bin", 0x0000, 0x1000, CRC(569dd60c) SHA1(47b810bcb5a063ffb3034fd7138dc5e15d243676) ) // 5" 25-pin
	ROM_LOAD_OPTIONAL( "unidiskh.bin", 0x0000, 0x1000, CRC(5079ad85) SHA1(42bb91318f13929c3a440de3fa1f0491a0b90863) ) // 5" 34-pin
	ROM_LOAD_OPTIONAL( "unidisk8.bin", 0x0000, 0x1000, CRC(d04e6a43) SHA1(8db504d46ff0355c72bd58fd536abeb17425c532) ) // 8"

	// Xebec
	ROM_LOAD_OPTIONAL( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038 (http://stason.org/TULARC/pc/hard-drives-hdd/seagate/ST4038-1987-31MB-5-25-FH-MFM-ST412.html)
	ROM_LOAD_OPTIONAL( "st225.bin",    0x0000, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225 (http://stason.org/TULARC/pc/hard-drives-hdd/seagate/ST225-21MB-5-25-HH-MFM-ST412.html)

	// ABC 850
	ROM_LOAD_OPTIONAL( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime RO202 (http://stason.org/TULARC/pc/hard-drives-hdd/rodime/RO202-11MB-5-25-FH-MFM-ST506.html)
	ROM_LOAD_OPTIONAL( "basf6185.bin", 0x0000, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185 (http://stason.org/TULARC/pc/hard-drives-hdd/basf-magnetics/6185-22MB-5-25-FH-MFM-ST412.html)
	// ABC 852
	ROM_LOAD_OPTIONAL( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126 (http://stason.org/TULARC/pc/hard-drives-hdd/nec/D5126-20MB-5-25-HH-MFM-ST506.html)
	// ABC 856
	ROM_LOAD_OPTIONAL( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325 (http://stason.org/TULARC/pc/hard-drives-hdd/micropolis/1325-69MB-5-25-FH-MFM-ST506.html)

	// ABC 832
	ROM_LOAD_OPTIONAL( "micr 1.4.7c", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015 (v1.4)
	ROM_LOAD_OPTIONAL( "micr 2.3.7c", 0x0000, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115 (v2.3)
	ROM_LOAD_OPTIONAL( "basf 1.2.7c", 0x0000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118 (v1.2)
	// ABC 838
	ROM_LOAD_OPTIONAL( "basf 8 1.0.7c", 0x0000, 0x0800, NO_DUMP ) // BASF 6104, BASF 6115 (v1.0)
	// ABC 830
	ROM_LOAD_OPTIONAL( "basf .02.7c", 0x0000, 0x0800, CRC(5daba200) SHA1(7881933760bed3b94f27585c0a6fc43e5d5153f5) ) // BASF 6106/08
	ROM_LOAD( "mpi .02.7c", 0x0000, 0x0800, CRC(2aac9296) SHA1(c01a62e7933186bdf7068d2e9a5bc36590544349) ) // MPI 51 (55 10760-01)
	ROM_LOAD( "new mpi .02.7c", 0x0000, 0x0800, CRC(ab788171) SHA1(c8e29965c04c85f2f2648496ea10c9c7ff95392f) ) // MPI 51 (55 10760-01)
ROM_END

ROM_START( luxor_55_21046 )
	ROM_REGION( 0x4000, "conkort", ROMREGION_LOADBYNAME ) // A13 is always high, thus loading at 0x2000
	ROM_LOAD_OPTIONAL( "cntr 108.6cd", 0x2000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // 1986-03-12
	ROM_LOAD_OPTIONAL( "diab 207.6cd", 0x2000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // 1987-06-24
	ROM_LOAD( "cntr 1.07 6490318-07.6cd", 0x0000, 0x4000, CRC(db8c1c0e) SHA1(8bccd5bc72124984de529ee058df779f06d2c1d5) ) // 1985-07-03
ROM_END

/*-------------------------------------------------
    DEVICE_START( luxor_55_10828 )
-------------------------------------------------*/

static DEVICE_START( luxor_55_10828 )
{
	slow_t *conkort = get_safe_token_slow(device);
	const conkort_config *config = get_safe_config(device);

	// find our CPU 
	conkort->cpu = device->subdevice(Z80_TAG);

	// find devices
	conkort->bus = device->machine->device(config->bus_tag);
	conkort->z80pio = device->subdevice(Z80PIO_TAG);
	conkort->fd1791 = device->subdevice(FD1791_TAG);
	conkort->image0 = device->subdevice(FLOPPY_0);
	conkort->image1 = device->subdevice(FLOPPY_1);

	// register for state saving
	state_save_register_device_item(device, 0, conkort->cs);
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
	state_save_register_device_item(device, 0, conkort->fdc_drq);
	state_save_register_device_item(device, 0, conkort->wait_enable);
	state_save_register_device_item(device, 0, conkort->sel0);
	state_save_register_device_item(device, 0, conkort->sel1);

	// patch out protection checks
	UINT8 *rom = device->subregion("abc830")->base();
	rom[0x00fa] = 0xff;
	rom[0x0336] = 0xff;
	rom[0x0718] = 0xff;
	rom[0x072c] = 0xff;
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

static DEVICE_GET_INFO( luxor_55_10828 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(conkort_config);							break;
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
    DEVICE_GET_INFO( abc830_pio )
-------------------------------------------------*/

DEVICE_GET_INFO( abc830_pio )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc830_pio);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC 830");							break;

		default:                                        DEVICE_GET_INFO_CALL(luxor_55_10828);						break;
	}
}

/*-------------------------------------------------
    DEVICE_START( luxor_55_21046 )
-------------------------------------------------*/

static DEVICE_START( luxor_55_21046 )
{
	fast_t *conkort = get_safe_token_fast(device);
	const conkort_config *config = get_safe_config(device);

	// find our CPU
	conkort->cpu = device->subdevice(Z80_TAG);

	// find devices
	conkort->bus = device->machine->device(config->bus_tag);
	conkort->z80dma = device->subdevice(Z80DMA_TAG);
	conkort->wd1793 = device->subdevice(SAB1793_TAG);
	conkort->image0 = device->subdevice(FLOPPY_0);
	conkort->image1 = device->subdevice(FLOPPY_1);

	// set initial values
	conkort->dma_irq = 0;
	conkort->fdc_irq = 0;
	conkort->busy = 0;

	// inherit DIP switch settings
	conkort->sw1 = config->sw1;
	conkort->sw2 = config->sw2;
	conkort->sw3 = config->address;

	// register for state saving
	state_save_register_device_item(device, 0, conkort->cs);
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data_in);
	state_save_register_device_item(device, 0, conkort->data_out);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
	state_save_register_device_item(device, 0, conkort->dma_irq);
	state_save_register_device_item(device, 0, conkort->busy);

	// patch out sector skew table
	UINT8 *rom = device->subregion("conkort")->base();

	for (int i = 0; i < 16; i++)
		rom[0x2dd3 + i] = i + 1;
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

static DEVICE_GET_INFO( luxor_55_21046 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(conkort_config);							break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(fast_t);									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(luxor_55_21046);					break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(luxor_55_21046);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_21046);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(luxor_55_21046);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor Conkort 55 21046-41");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abc830 )
-------------------------------------------------*/

DEVICE_GET_INFO( abc830 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc830);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC 830");							break;

		default:                                        DEVICE_GET_INFO_CALL(luxor_55_21046);						break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abc832 )
-------------------------------------------------*/

DEVICE_GET_INFO( abc832 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc832);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC 832");							break;

		default:                                        DEVICE_GET_INFO_CALL(luxor_55_21046);						break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abc838 )
-------------------------------------------------*/

DEVICE_GET_INFO( abc838 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc838);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC 838");							break;

		default:                                        DEVICE_GET_INFO_CALL(luxor_55_21046);						break;
	}
}

DEFINE_LEGACY_DEVICE(ABC830_PIO, abc830_pio);
DEFINE_LEGACY_DEVICE(ABC830, abc830);
DEFINE_LEGACY_DEVICE(ABC832, abc832);
DEFINE_LEGACY_DEVICE(ABC838, abc838);
