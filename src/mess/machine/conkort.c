/*

Luxor Conkort

PCB Layout
----------

55 21046-03

|-----------------------------------|
|	LD1	SW1				CON2		|
|									|
|						CON3	S9	|
|								S8	|
|	LS240	LS174	7406	7406	|
|									|
|	LS174	FDC9229	LS107	LS266	|
|									|
|									|
|		SAB1793						|
|					LS368	16MHz	|
|					S6				|
|		Z80DMA			ROM			|
|									|
|									|
|		Z80				TC5565		|
|									|
|									|
|	LS138	LS174	SW2		74374	|
|									|
|	LS10	LS266	S5		LS374	|
|					S3				|
|					S1		LS240	|
|									|
|			LS244	SW3		DM8131	|
|									|
|									|
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM     - Toshiba TMM27128D-20 "CNTR 1.07 6490318-07"
    TC5565  - Toshiba TC5565PL-15 8192x8 bit Static Random Access Memory
    Z80	    - Zilog Z8400APS Z80A CPU Central Processing Unit
    Z80DMA  - Zilog Z8410APS Z80A DMA Direct Memory Access Controller
    SAB1793 - Siemens SAB1793-02P Floppy Disc Controller
    FDC9229 - SMC FDC9229BT Floppy Disc Interface Circuit
    DM8131  - National Semiconductor DM8131N 6-Bit Unified Bus Comparator
    CON1	- 
    CON2	- AMP4284
    CON3	- 
    SW1		- 
    SW2		- 
    SW3		- 
    S1		- 
   	S3		- 
    S5		- 
    S6		- Amount of RAM installed (A:2KB, B:8KB)
    S7		- Number of drives connected (0:3, 1:?) *located on solder side
    S8		- 0:8", 1:5.25"
    S9		- Location of RDY signal (A:P2:6, B:P2:34)
    LD1		- LED

*/

/*

	TODO:

	- install read/write handlers for ABCBUS
	- floppy drive selection
	- floppy side selection
	- DS/DD SS/DS jumpers
	- S1-S5 jumpers
	- read C5 from DS8131 (card address)
	- protection device @ 8B
	- Z80 wait logic
	- everything for fast controller

*/

#include "driver.h"
#include "conkort.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/wd17xx.h"

typedef struct _conkort_t conkort_t;
struct _conkort_t
{
	int cpunum;						/* CPU index of the Z80 */

	UINT8 status;
	UINT8 data;
	int pio_ardy;
	int fdc_irq;
};

INLINE conkort_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (conkort_t *)device->token;
}

/* Slow Controller */

READ8_HANDLER( luxor_55_10828_data_r )
{
	const device_config *device = devtag_get_device(machine, LUXOR_55_10828, CONKORT_TAG);
	const device_config *z80pio = devtag_get_device(machine, Z80PIO, CONKORT_Z80PIO_TAG);

	conkort_t *conkort = get_safe_token(device);

	UINT8 data = 0xff;

	z80pio_astb_w(z80pio, 0);

	if (!BIT(conkort->status, 6))
	{
		data = conkort->data;
	}

	z80pio_astb_w(z80pio, 1);

	return data;
}

WRITE8_HANDLER( luxor_55_10828_data_w )
{
	const device_config *device = devtag_get_device(machine, LUXOR_55_10828, CONKORT_TAG);
	const device_config *z80pio = devtag_get_device(machine, Z80PIO, CONKORT_Z80PIO_TAG);

	conkort_t *conkort = get_safe_token(device);

	z80pio_astb_w(z80pio, 0);

	if (BIT(conkort->status, 6))
	{
		conkort->data = data;
	}

	z80pio_astb_w(z80pio, 1);
}

READ8_HANDLER( luxor_55_10828_status_r )
{
	const device_config *device = devtag_get_device(machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(device);

	return (conkort->pio_ardy << 7) | conkort->status;
}

WRITE8_HANDLER( luxor_55_10828_channel_w )
{
}

WRITE8_HANDLER( luxor_55_10828_command_w )
{
}

READ8_HANDLER( luxor_55_10828_reset_r )
{
	// reset Z80 and slow_ctrl lach

	return 0xff;
}

READ8_HANDLER( slow_ctrl_r )
{
	/*

		bit		description

		0		_SEL 0
		1		_SEL 1
		2		_SEL 2
		3		_MOT
		4		_SIDE
		5		8B pin 2 or GND
		6		8A pin 13
		7		FDC _MR

	*/

	return 0;
}

WRITE8_HANDLER( slow_ctrl_w )
{
}

WRITE8_HANDLER( slow_status_w )
{
	const device_config *device = devtag_get_device(machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(device);

	conkort->status = data & 0x7f;

	cpunum_set_input_line(device->machine, 0, INPUT_LINE_IRQ0, BIT(data, 7) ? ASSERT_LINE : CLEAR_LINE);
}

/* Fast Controller */


READ8_HANDLER( luxor_55_21046_data_r )
{
	return 0;
}

WRITE8_HANDLER( luxor_55_21046_data_w )
{
}

READ8_HANDLER( luxor_55_21046_status_r )
{
	return 0;
}

WRITE8_HANDLER( luxor_55_21046_channel_w )
{
}

WRITE8_HANDLER( luxor_55_21046_command_w )
{
}

READ8_HANDLER( luxor_55_21046_reset_r )
{
	return 0;
}

/* Memory Maps */

// Slow Controller

static ADDRESS_MAP_START( luxor_55_10828_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x13ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( luxor_55_10828_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_WRITE(slow_status_w)
	AM_RANGE(0x20, 0x20) AM_READWRITE(slow_ctrl_r, slow_ctrl_w)
	AM_RANGE(0x40, 0x43) AM_READWRITE(wd17xx_r, wd17xx_w)
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE(Z80PIO, CONKORT_Z80PIO_TAG, z80pio_alt_r, z80pio_alt_w)
ADDRESS_MAP_END

// Fast Controller

static ADDRESS_MAP_START( luxor_55_21046_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( luxor_55_21046_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
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
INPUT_PORTS_END

/* Z80 PIO */

static Z80PIO_ON_INT_CHANGED( pio_interrupt )
{
	cpunum_set_input_line(device->machine, 1, INPUT_LINE_IRQ0, state); // TODO hardcoded
}

static READ8_DEVICE_HANDLER( pio_port_a_r )
{
	/*

		bit		description

		0		_INT to main Z80
		1		
		2		
		3		
		4		
		5		
		6		LS245 DIR
		7		

	*/

	const device_config *conkort_device = devtag_get_device(device->machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(conkort_device);

	return conkort->data;
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	const device_config *conkort_device = devtag_get_device(device->machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(conkort_device);

	conkort->data = data;
}

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	/*

		bit		description

		0		!(_DS0 & _DS1)
		1		!(_DD0 & _DD1)
		2		8B pin 10
		3		FDC DDEN
		4		_R/BS
		5		FDC HLT
		6		FDC _HDLD
		7		FDC IRQ

	*/

	const device_config *conkort_device = devtag_get_device(device->machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(conkort_device);

	return conkort->fdc_irq << 7;
}

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
	/*

		bit		description

		0		!(_DS0 & _DS1)
		1		!(_DD0 & _DD1)
		2		8B pin 10
		3		FDC DDEN
		4		_R/BS
		5		FDC HLT
		6		FDC _HDLD
		7		FDC IRQ

	*/

}

static Z80PIO_ON_ARDY_CHANGED( pio_ardy_w )
{
	const device_config *conkort_device = devtag_get_device(device->machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(conkort_device);

	conkort->pio_ardy = state;
}

static Z80PIO_INTERFACE( pio_intf )
{
	CONKORT_Z80_TAG,			/* CPU */
	0,							/* clock (get from main CPU) */
	pio_interrupt,				/* callback when change interrupt status */
	pio_port_a_r,				/* port A read callback */
	pio_port_b_r,				/* port B read callback */
	pio_port_a_w,				/* port A write callback */
	pio_port_b_w,				/* port B write callback */
	pio_ardy_w,					/* portA ready active callback */
	NULL						/* portB ready active callback */
};

static const z80_daisy_chain luxor_55_10828_daisy_chain[] =
{
	{ Z80PIO, CONKORT_Z80PIO_TAG },
	{ NULL }
};

/* Z80 DMA */

static READ8_DEVICE_HANDLER( dma_read_byte )
{
	UINT8 data;

	cpuintrf_push_context(0);
	data = program_read_byte(offset);
	cpuintrf_pop_context();

	return data;
}

static WRITE8_DEVICE_HANDLER( dma_write_byte )
{
	cpuintrf_push_context(0);
	program_write_byte(offset, data);
	cpuintrf_pop_context();
}

static READ8_DEVICE_HANDLER( wd17xx_read_byte )
{
	return wd17xx_data_r(device->machine, offset);
}

static WRITE8_DEVICE_HANDLER( wd17xx_write_byte )
{
	wd17xx_data_w(device->machine, offset, data);
}

static const z80dma_interface dma_intf =
{
	0,
	XTAL_16MHz/4, // ???
	dma_read_byte,
	dma_write_byte,
	wd17xx_read_byte,
	wd17xx_write_byte,
	0,
	0
};

static const z80_daisy_chain luxor_55_21046_daisy_chain[] =
{
	{ Z80DMA, CONKORT_Z80DMA_TAG },
	{ NULL }
};

/* FD1791 */

static void wd1791_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	const device_config *device = devtag_get_device(machine, LUXOR_55_10828, CONKORT_TAG);
	conkort_t *conkort = get_safe_token(device);

	switch(state)
	{
		case WD17XX_IRQ_CLR:
			conkort->fdc_irq = 0;
			break;
		case WD17XX_IRQ_SET:
			conkort->fdc_irq = 1;
			break;
		case WD17XX_DRQ_CLR:				
			break;
		case WD17XX_DRQ_SET:
			break;
	}
}

/* FD1793 */

static void wd1793_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	switch(state)
	{
		case WD17XX_IRQ_CLR:
			break;
		case WD17XX_IRQ_SET:
			break;
		case WD17XX_DRQ_CLR:				
			break;
		case WD17XX_DRQ_SET:
			break;
	}
}

/* Machine Start */

static MACHINE_START( luxor_55_10828 )
{
	wd17xx_init(machine, WD_TYPE_179X, wd1791_callback, NULL); // FD1791-01
}

static MACHINE_START( luxor_55_21046 )
{
	wd17xx_init(machine, WD_TYPE_1793, wd1793_callback, NULL); // FD1793-01
}

/* Machine Driver */

MACHINE_DRIVER_START( luxor_55_10828 )
	MDRV_CPU_ADD(CONKORT_Z80_TAG, Z80, XTAL_4MHz/2)
	MDRV_CPU_PROGRAM_MAP(luxor_55_10828_map, 0)
	MDRV_CPU_IO_MAP(luxor_55_10828_io_map, 0)
	MDRV_CPU_CONFIG(luxor_55_10828_daisy_chain)

	MDRV_Z80PIO_ADD(CONKORT_Z80PIO_TAG, pio_intf)
	
	MDRV_MACHINE_START(luxor_55_10828)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( luxor_55_21046 )
	MDRV_CPU_ADD(CONKORT_Z80_TAG, Z80, XTAL_16MHz/4)
	MDRV_CPU_PROGRAM_MAP(luxor_55_21046_map, 0)
	MDRV_CPU_IO_MAP(luxor_55_21046_io_map, 0)
	MDRV_CPU_CONFIG(luxor_55_21046_daisy_chain)

	MDRV_DEVICE_ADD(CONKORT_Z80DMA_TAG, Z80DMA)
	MDRV_DEVICE_CONFIG(dma_intf)
	
	MDRV_MACHINE_START(luxor_55_21046)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( l5510828 )
	ROM_REGION( 0x10000, CONKORT_Z80_TAG, 0 )

	ROM_REGION( 0x800, "abc830", 0 )
	ROM_LOAD( "mpi02.bin",    0x0000, 0x0800, CRC(2aac9296) SHA1(c01a62e7933186bdf7068d2e9a5bc36590544349) ) // ABC830 with MPI drives. Styrkort Artnr 5510760-01

	ROM_REGION( 0x1800, "abc832", 0 )
	ROM_LOAD( "micr1015.bin", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015
	ROM_LOAD( "micr1115.bin", 0x0800, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115
	ROM_LOAD( "basf6118.bin", 0x1000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118

	ROM_REGION( 0x1000, "abc850", 0 )
	ROM_LOAD( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime 202
	ROM_LOAD( "basf6185.bin", 0x0800, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185

	ROM_REGION( 0x1000, "abc852", 0 )
	ROM_LOAD( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126

	ROM_REGION( 0x800, "abc856", 0 )
	ROM_LOAD( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325

	ROM_REGION( 0x1000, "xebec", 0 )
	ROM_LOAD( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038
	ROM_LOAD( "st225.bin",    0x0800, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225
ROM_END

ROM_START( l5521046 )
	ROM_REGION( 0x10000, CONKORT_Z80_TAG, 0 )
	ROM_LOAD( "fast108.bin",	0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",	0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07
	ROM_LOAD( "6490318-07.bin", 0x0000, 0x2000, CRC(06ae1fe8) SHA1(ad1d9d0c192539af70cb95223263915a09693ef8) ) // PROM v1.07, Art N/O 6490318-07. Luxor Styrkort Art. N/O 55 21046-41. Date 1985-07-03
ROM_END

/* Device Interface */

static DEVICE_START( luxor_55_10828 )
{
	conkort_t *conkort = device->token;
	char unique_tag[30];
	astring *tempstring = astring_alloc();

	/* validate arguments */

	assert(device->machine != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	/* find our CPU */

	astring_printf(tempstring, "%s:%s", device->tag, CONKORT_Z80_TAG);
	conkort->cpunum = mame_find_cpu_index(device->machine, astring_c(tempstring));
	astring_free(tempstring);

	/* register for state saving */

	state_save_combine_module_and_tag(unique_tag, "conkort", device->tag);

	return DEVICE_START_OK;
}

static DEVICE_SET_INFO( luxor_55_10828 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( luxor_55_10828 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(conkort_t);				break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = rom_l5510828;				break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = machine_config_luxor_55_10828; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(luxor_55_10828); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_10828);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Luxor 55 10828";					break;
		case DEVINFO_STR_FAMILY:						info->s = "Luxor ABC";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}

static DEVICE_START( luxor_55_21046 )
{
	conkort_t *conkort = device->token;
	char unique_tag[30];
	astring *tempstring = astring_alloc();

	/* validate arguments */

	assert(device->machine != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	/* find our CPU */

	astring_printf(tempstring, "%s:%s", device->tag, CONKORT_Z80_TAG);
	conkort->cpunum = mame_find_cpu_index(device->machine, astring_c(tempstring));
	astring_free(tempstring);

	/* register for state saving */

	state_save_combine_module_and_tag(unique_tag, "conkort", device->tag);

	return DEVICE_START_OK;
}

static DEVICE_SET_INFO( luxor_55_21046 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( luxor_55_21046 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(conkort_t);				break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = rom_l5521046;				break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = machine_config_luxor_55_21046; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(luxor_55_21046); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(luxor_55_21046);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Luxor Conkort 55 21046";			break;
		case DEVINFO_STR_FAMILY:						info->s = "Luxor ABC";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}
