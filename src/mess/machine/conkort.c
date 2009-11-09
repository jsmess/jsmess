/*

Luxor Conkort

PCB Layout
----------

55 21046-03

|-----------------------------------|
|   LD1 SW1             CON2        |
|                                   |
|                       CON3    S9  |
|                               S8  |
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
    Z80     - Zilog Z8400APS Z80A CPU Central Processing Unit
    Z80DMA  - Zilog Z8410APS Z80A DMA Direct Memory Access Controller
    SAB1793 - Siemens SAB1793-02P Floppy Disc Controller
    FDC9229 - SMC FDC9229BT Floppy Disc Interface Circuit
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    CON1    -
    CON2    - AMP4284
    CON3    -
    SW1     -
    SW2     -
    SW3     -
    S1      -
    S3      -
    S5      -
    S6      - Amount of RAM installed (A:2KB, B:8KB)
    S7      - Number of drives connected (0:3, 1:?) *located on solder side
    S8      - 0:8", 1:5.25"
    S9      - Location of RDY signal (A:P2:6, B:P2:34)
    LD1     - LED

*/

/*

    TODO:

    Slow Controller
    ---------------
    - DS/DD SS/DS jumpers
    - S1-S5 jumpers
    - protection device @ 8B
    - FDC DRQ
    - Z80 wait logic

    Fast Controller
    ---------------
    - implement missing features to Z80DMA
    - FDC INT
    - FDC DRQ

*/

#include "driver.h"
#include "conkort.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/wd17xx.h"
#include "machine/abcbus.h"
#include "devices/flopdrv.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define Z80_TAG		"5a"
#define Z80PIO_TAG	"3a"
#define Z80DMA_TAG	"z80dma"
#define WD1791_TAG	"wd1791"
#define SAB1793_TAG	"sab1793"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _slow_t slow_t;
struct _slow_t
{
	UINT8 status;			/* ABC BUS status */
	UINT8 data;				/* ABC BUS data */
	int pio_ardy;			/* PIO port A ready */
	int fdc_irq;			/* FDC interrupt */

	/* devices */
	const device_config *cpu;
	const device_config *z80pio;
	const device_config *wd1791;
};

typedef struct _fast_t fast_t;
struct _fast_t
{
	UINT8 status;			/* ABC BUS status */
	UINT8 data;				/* ABC BUS data */
	int fdc_irq;			/* FDC interrupt */

	/* devices */
	const device_config *cpu;
	const device_config *z80dma;
	const device_config *wd1793;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE slow_t *get_safe_token_slow(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (slow_t *)device->token;
}

INLINE fast_t *get_safe_token_fast(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (fast_t *)device->token;
}

INLINE slow_t *get_safe_token_machine_slow(running_machine *machine)
{
   	const device_config *device = devtag_get_device(machine, CONKORT_TAG);
	return get_safe_token_slow(device);
}

INLINE fast_t *get_safe_token_machine_fast(running_machine *machine)
{
   	const device_config *device = devtag_get_device(machine, CONKORT_TAG);
	return get_safe_token_fast(device);
}

INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* Slow Controller */

static READ8_HANDLER( slow_bus_data_r )
{
	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	UINT8 data = 0xff;

	z80pio_astb_w(conkort->z80pio, 0);

	if (!BIT(conkort->status, 6))
	{
		data = conkort->data;
	}

	z80pio_astb_w(conkort->z80pio, 1);

	return data;
}

static WRITE8_HANDLER( slow_bus_data_w )
{
	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	z80pio_astb_w(conkort->z80pio, 0);

	if (BIT(conkort->status, 6))
	{
		conkort->data = data;
	}

	z80pio_astb_w(conkort->z80pio, 1);
}

static READ8_HANDLER( slow_bus_stat_r )
{
	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	return (conkort->status & 0xfe) | conkort->pio_ardy;
}

static WRITE8_HANDLER( slow_bus_c1_w )
{
	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( slow_bus_c3_w )
{
	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
}

static ABCBUS_CARD_SELECT( luxor_55_10828 )
{
	slow_t *conkort = get_safe_token_slow(device);

	if (data == 0x2d) // TODO: bit 0 of this is configurable with S1
	{
		const address_space *io = cpu_get_address_space(device->machine->firstcpu, ADDRESS_SPACE_IO);

		memory_install_readwrite8_handler(io, ABCBUS_INP, ABCBUS_OUT, 0x18, 0, slow_bus_data_r, slow_bus_data_w);
		memory_install_read8_handler(io, ABCBUS_STAT, ABCBUS_STAT, 0x18, 0, slow_bus_stat_r);
		memory_install_write8_handler(io, ABCBUS_C1, ABCBUS_C1, 0x18, 0, slow_bus_c1_w);
		memory_install_write8_handler(io, ABCBUS_C2, ABCBUS_C2, 0x18, 0, SMH_NOP);
		memory_install_write8_handler(io, ABCBUS_C3, ABCBUS_C3, 0x18, 0, slow_bus_c3_w);
		memory_install_write8_handler(io, ABCBUS_C4, ABCBUS_C4, 0x18, 0, SMH_NOP);
	}

	cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
}

static WRITE8_HANDLER( slow_ctrl_w )
{
	/*

        bit     description

        0       _SEL 0
        1       _SEL 1
        2       _SEL 2
        3       _MOT
        4       _SIDE
        5       8B pin 2 (if S4 shorted)
        6       8A pin 13
        7       FDC _MR

    */

	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	/* drive selection */
	if (!BIT(data, 0)) wd17xx_set_drive(conkort->wd1791, 0);
	if (!BIT(data, 1)) wd17xx_set_drive(conkort->wd1791, 1);
//  if (!BIT(data, 2)) wd17xx_set_drive(conkort->wd1791, 2);

	/* motor enable */
	floppy_drive_set_motor_state(get_floppy_image(space->machine, 0), !BIT(data, 3));
	floppy_drive_set_motor_state(get_floppy_image(space->machine, 1), !BIT(data, 3));
	floppy_drive_set_ready_state(get_floppy_image(space->machine, 0), 1, 1);
	floppy_drive_set_ready_state(get_floppy_image(space->machine, 1), 1, 1);

	/* disk side selection */
	wd17xx_set_side(conkort->wd1791,!BIT(data, 4));

	if (!BIT(data, 7))
	{
		/* FDC master reset */
		wd17xx_reset(conkort->wd1791);
	}
}

static WRITE8_HANDLER( slow_status_w )
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

	slow_t *conkort = get_safe_token_machine_slow(space->machine);

	conkort->status = data;

	/* interrupt to main CPU */
	cpu_set_input_line(conkort->cpu, INPUT_LINE_IRQ0, BIT(data, 7) ? ASSERT_LINE : CLEAR_LINE);
}

/* Fast Controller */

static READ8_HANDLER( fast_bus_data_r )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	UINT8 data = 0xff;

	if (!BIT(conkort->status, 6))
	{
		data = conkort->data;
	}

	return data;
}

static WRITE8_HANDLER( fast_bus_data_w )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	if (BIT(conkort->status, 6))
	{
		conkort->data = data;
	}
}

static READ8_HANDLER( fast_bus_stat_r )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	return conkort->status;
}

static WRITE8_HANDLER( fast_bus_c1_w )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	cpu_set_input_line(conkort->cpu, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( fast_bus_c3_w )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
}

static ABCBUS_CARD_SELECT( luxor_55_21046 )
{
	fast_t *conkort = get_safe_token_fast(device);

	if (data == input_port_read(device->machine, "SW3"))
	{
		const address_space *io = cpu_get_address_space(device->machine->firstcpu, ADDRESS_SPACE_IO);

		memory_install_readwrite8_handler(io, ABCBUS_INP, ABCBUS_OUT, 0x18, 0, fast_bus_data_r, fast_bus_data_w);
		memory_install_read8_handler(io, ABCBUS_STAT, ABCBUS_STAT, 0x18, 0, fast_bus_stat_r);
		memory_install_write8_handler(io, ABCBUS_C1, ABCBUS_C1, 0x18, 0, fast_bus_c1_w);
		memory_install_write8_handler(io, ABCBUS_C2, ABCBUS_C2, 0x18, 0, SMH_NOP);
		memory_install_write8_handler(io, ABCBUS_C3, ABCBUS_C3, 0x18, 0, fast_bus_c3_w);
		memory_install_write8_handler(io, ABCBUS_C4, ABCBUS_C4, 0x18, 0, SMH_NOP);
	}

	cpu_set_input_line(conkort->cpu, INPUT_LINE_RESET, PULSE_LINE);
}

static READ8_HANDLER( fast_ctrl_r )
{
	/*

        bit     description

        0       
        1
        2
        3		? (must be 1 to boot)
        4
        5
        6       
        7

    */

	return 0x08;
}

static READ8_HANDLER( fast_data_r )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	return conkort->data;
}

static WRITE8_HANDLER( fast_data_w )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	conkort->data = data;
}

static WRITE8_HANDLER( fast_status_w )
{
	fast_t *conkort = get_safe_token_machine_fast(space->machine);

	conkort->status = data;
}

/* Memory Maps */

// Slow Controller

static ADDRESS_MAP_START( slow_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_REGION("abc830", 0)
	AM_RANGE(0x1000, 0x13ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( slow_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x7c, 0x7f) AM_DEVREADWRITE(Z80PIO_TAG, z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0xbc, 0xbf) AM_DEVREADWRITE(WD1791_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xdf, 0xdf) AM_WRITE(slow_status_w)
	AM_RANGE(0xef, 0xef) AM_WRITE(slow_ctrl_w)
ADDRESS_MAP_END

// Fast Controller

static ADDRESS_MAP_START( fast_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM AM_REGION("conkort", 0)
	AM_RANGE(0x2000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fast_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x0f) AM_READ(fast_data_r)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0f) AM_WRITE(fast_data_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x0f) AM_WRITE(fast_status_w)
//  AM_RANGE(0x30, 0x30) AM_MIRROR(0x0f) AM_WRITE()
//  AM_RANGE(0x40, 0x40) AM_MIRROR(0x0f) AM_WRITE()
	AM_RANGE(0x50, 0x50) AM_MIRROR(0x0f) AM_READ(fast_ctrl_r)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0x0c) AM_DEVREAD(SAB1793_TAG, wd17xx_r)
	AM_RANGE(0x70, 0x73) AM_MIRROR(0x0c) AM_DEVWRITE(SAB1793_TAG, wd17xx_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) AM_DEVREADWRITE(Z80DMA_TAG, z80dma_r, z80dma_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( luxor_55_21046 )
	PORT_START("SW1")
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

	PORT_START("SW2")
	PORT_DIPNAME( 0x0f, 0x08, "Disk Drive" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x08, "BASF 6106/08 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x09, "MPI 51 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x04, "BASF 6118 (ABC 832, 190 9711-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x03, "Micropolis 1015F (ABC 832, 190 9711-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x05, "Micropolis 1115F (ABC 832, 190 9711-17)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x01, "TEAC FD55F (ABC 834, 230 7802-01)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x02, "BASF 6138 (ABC 850, 230 8440-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x0e, "BASF 6105" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPSETTING(    0x0f, "BASF 6106 (ABC 838, 230 8838-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)

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
	slow_t *conkort = get_safe_token_machine_slow(device->machine);

	return conkort->data;
}

static WRITE8_DEVICE_HANDLER( conkort_pio_port_a_w )
{
	slow_t *conkort = get_safe_token_machine_slow(device->machine);

	conkort->data = data;
}

static READ8_DEVICE_HANDLER( conkort_pio_port_b_r )
{
	/*

        bit     description

        0       !(_DS0 & _DS1)
        1       !(_DD0 & _DD1)
        2       8B pin 10
        3       FDC DDEN
        4       _R/BS
        5       FDC HLT
        6       FDC _HDLD
        7       FDC IRQ

    */

	slow_t *conkort = get_safe_token_machine_slow(device->machine);

	return conkort->fdc_irq << 7;
}

static WRITE8_DEVICE_HANDLER( conkort_pio_port_b_w )
{
	/*

        bit     description

        0       !(_DS0 & _DS1)
        1       !(_DD0 & _DD1)
        2       8B pin 10
        3       FDC DDEN
        4       _R/BS
        5       FDC HLT
        6       FDC _HDLD
        7       FDC IRQ

    */

}

static WRITE_LINE_DEVICE_HANDLER( conkort_pio_ardy_w )
{
	slow_t *conkort = get_safe_token_machine_slow(device->machine);

	conkort->pio_ardy = state;
}

static const z80pio_interface conkort_pio_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),		/* interrupt callback */
	DEVCB_HANDLER(conkort_pio_port_a_r),	/* port A read callback */
	DEVCB_HANDLER(conkort_pio_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(conkort_pio_port_a_w),	/* port A write callback */
	DEVCB_HANDLER(conkort_pio_port_b_w),	/* port B write callback */
	DEVCB_LINE(conkort_pio_ardy_w),			/* port A ready callback */
	DEVCB_NULL								/* port B ready callback */
};

static const z80_daisy_chain slow_daisy_chain[] =
{
	{ Z80PIO_TAG },
	{ NULL }
};

/* Z80 DMA */

/*

    DMA Transfer Programs
    ---------------------

    C3 14 28 95 6B 02 8A CF 01 AF CF 87

    C3  reset
    14  port A is memory, port A address increments
    28  port B is I/O, port B address fixed
    95  byte mode, port B starting address low follows, interrupt control byte follows
    6B  port B starting address 0x006B (FDC DATA read)
    02  interrupt at end of block
    8A  ready active high
    CF  load
    01  transfer B->A
    AF  disable interrupts
    CF  load
    87  enable DMA

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

    C3 91 40 8A AB

    C3  reset
    91  byte mode, interrupt control byte follows
    40  interrupt on RDY
    8A  _CE only, ready active high, stop on end of block
    AB  enable interrupts

*/

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

static const z80_daisy_chain fast_daisy_chain[] =
{
	{ Z80DMA_TAG },
	{ NULL }
};

/* FD1791 */

static WRITE_LINE_DEVICE_HANDLER( slow_wd1791_intrq_w )
{
	slow_t *conkort = get_safe_token_machine_slow(device->machine);

	conkort->fdc_irq = state;
}

static const wd17xx_interface slow_wd17xx_interface =
{
	DEVCB_LINE(slow_wd1791_intrq_w),
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* FD1793 */

static const wd17xx_interface fast_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z80DMA_TAG, z80dma_rdy_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* Machine Driver */

static MACHINE_DRIVER_START( luxor_55_10828 )
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz/2)
	MDRV_CPU_PROGRAM_MAP(slow_map)
	MDRV_CPU_IO_MAP(slow_io_map)
	MDRV_CPU_CONFIG(slow_daisy_chain)

	MDRV_Z80PIO_ADD(Z80PIO_TAG, conkort_pio_intf)
	MDRV_WD179X_ADD(WD1791_TAG, slow_wd17xx_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( luxor_55_21046 )
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
	MDRV_CPU_PROGRAM_MAP(fast_map)
	MDRV_CPU_IO_MAP(fast_io_map)
	MDRV_CPU_CONFIG(fast_daisy_chain)

	MDRV_Z80DMA_ADD(Z80DMA_TAG, XTAL_16MHz/4, dma_intf)
	MDRV_WD1793_ADD(SAB1793_TAG, fast_wd17xx_interface)
MACHINE_DRIVER_END

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
	ROM_LOAD( "fast108.bin",	0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",	0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07
	ROM_LOAD( "6490318-07.bin", 0x0000, 0x2000, CRC(06ae1fe8) SHA1(ad1d9d0c192539af70cb95223263915a09693ef8) ) // PROM v1.07, Art N/O 6490318-07. Luxor Styrkort Art. N/O 55 21046-41. Date 1985-07-03
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
	slow_t *conkort = device->token;

	/* find our CPU */
	conkort->cpu = device_find_child_by_tag(device, Z80_TAG);

	/* find devices */
	conkort->z80pio = device_find_child_by_tag(device, Z80PIO_TAG);
	conkort->wd1791 = device_find_child_by_tag(device, WD1791_TAG);

	/* register for state saving */
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data);
	state_save_register_device_item(device, 0, conkort->pio_ardy);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
}

/*-------------------------------------------------
    DEVICE_RESET( luxor_55_10828 )
-------------------------------------------------*/

static DEVICE_RESET( luxor_55_10828 )
{

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
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(slow_t);									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(luxor_55_10828);					break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(luxor_55_10828);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_10828);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(luxor_55_10828);			break;
		case DEVINFO_FCT_ABCBUS_CARD_SELECT:			info->f = (genf *)ABCBUS_CARD_SELECT_NAME(luxor_55_10828);	break;

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
	fast_t *conkort = device->token;

	/* find our CPU */
	conkort->cpu = device_find_child_by_tag(device, Z80_TAG);

	/* find devices */
	conkort->z80dma = device_find_child_by_tag(device, Z80DMA_TAG);
	conkort->wd1793 = device_find_child_by_tag(device, SAB1793_TAG);

	/* register for state saving */
	state_save_register_device_item(device, 0, conkort->status);
	state_save_register_device_item(device, 0, conkort->data);
	state_save_register_device_item(device, 0, conkort->fdc_irq);
}

/*-------------------------------------------------
    DEVICE_RESET( luxor_55_21046 )
-------------------------------------------------*/

static DEVICE_RESET( luxor_55_21046 )
{
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 0), 1);
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 1), 1);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 0), 1, 1);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 1), 1, 1);
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
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(fast_t);									break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(luxor_55_21046);					break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(luxor_55_21046);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_21046);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(luxor_55_21046);			break;
		case DEVINFO_FCT_ABCBUS_CARD_SELECT:			info->f = (genf *)ABCBUS_CARD_SELECT_NAME(luxor_55_21046);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor Conkort 55 21046-xx");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
