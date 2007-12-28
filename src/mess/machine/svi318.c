/*
** Spectravideo SVI-318 and SVI-328
**
** Sean Young, Tomas Karlsson
**
** Todo:
** - modem port
** - svi_cas format
*/

#include "driver.h"
#include "includes/svi318.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "video/crtc6845.h"
#include "video/tms9928a.h"
#include "machine/8255ppi.h"
#include "machine/uart8250.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "formats/svi_cas.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "image.h"

enum {
	SVI_INTERNAL	= 0,
	SVI_CART		= 1,
	SVI_EXPRAM2		= 2,
	SVI_EXPRAM3		= 3
};

typedef struct {
	/* general */
	UINT8	svi318;					/* Are we dealing with an SVI-318 or a SVI-328 model. 0 = 328, 1 = 318 */
	/* memory */
	UINT8	*empty_bank;
	UINT8	bank_switch;
	UINT8	bank1;
	UINT8	bank2;
	UINT8	*bank1_ptr;
	UINT8	bank1_read_only;
	UINT8	*bank2_ptr;
	UINT8	bank2_read_only;
	UINT8	*bank3_ptr;
	UINT8	bank3_read_only;
	/* printer */
	UINT8	prn_data;
	UINT8	prn_strobe;
	/* SVI-806 80 column card */
	UINT8	svi806_present;
	UINT8	svi806_ram_enabled;
	UINT8	*svi806_ram;
	UINT8	*svi806_gfx;
} SVI_318;

static SVI_318 svi;
static UINT8 *pcart;
static UINT32 pcart_rom_size;

static void svi318_set_banks (void);

/* Serial ports */

static void svi318_uart8250_interrupt(int nr, int state)
{
	cpunum_set_input_line(0, 0, (state ? HOLD_LINE : CLEAR_LINE));
}

static const uart8250_interface svi318_uart8250_interface[1] =
{
	{
		TYPE8250,
		3072000,
		svi318_uart8250_interrupt,
		NULL,
		NULL,
		NULL
	},
};

/* Cartridge */

static int svi318_verify_cart (UINT8 magic[2])
{
	/* read the first two bytes */
	if ( (magic[0] == 0xf3) && (magic[1] == 0x31) )
		return IMAGE_VERIFY_PASS;
	else
		return IMAGE_VERIFY_FAIL;
}

DEVICE_INIT( svi318_cart ) {
	pcart = NULL;
	pcart_rom_size = 0;

	return INIT_PASS;
}

DEVICE_LOAD( svi318_cart )
{
	UINT8 *p;
	UINT32 size;

	size = MAX(0x8000,image_length(image));

	p = image_malloc(image, size);
	if (!p)
		return INIT_FAIL;

	pcart_rom_size = size;
	memset(p, 0xff, size);

	size = image_length(image);
	if (image_fread(image, p, size) != size)
	{
		logerror ("can't read file %s\n", image_filename(image) );
		return INIT_FAIL;
	}

	if (svi318_verify_cart(p)==IMAGE_VERIFY_FAIL)
		return INIT_FAIL;
	pcart = p;

	return INIT_PASS;
}

DEVICE_UNLOAD( svi318_cart )
{
	pcart = NULL;
	pcart_rom_size = 0;
}

/* PPI */

/*
 PPI Port A Input (address 98H)
 Bit Name     Description
  1  TA       Joystick 1, /SENSE
  2  TB       Joystick 1, EOC
  3  TC       Joystick 2, /SENSE
  4  TD       Joystick 2, EOC
  5  TRIGGER1 Joystick 1, Trigger
  6  TRIGGER2 Joystick 2, Trigger
  7  /READY   Cassette, Ready
  8  CASR     Cassette, Read data
*/

static READ8_HANDLER ( svi318_ppi_port_a_r )
{
	int data = 0x0f;

	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038)
		data |= 0x80;
	if (!svi318_cassette_present(0))
		data |= 0x40;
	data |= readinputport(12) & 0x30;

	return data;
}

/*
 PPI Port B Input (address 99H)
 Bit Name Description
  1  IN0  Keyboard, Column status of selected line
  2  IN1  Keyboard, Column status of selected line
  3  IN2  Keyboard, Column status of selected line
  4  IN3  Keyboard, Column status of selected line
  5  IN4  Keyboard, Column status of selected line
  6  IN5  Keyboard, Column status of selected line
  7  IN6  Keyboard, Column status of selected line
  8  IN7  Keyboard, Column status of selected line
*/

static  READ8_HANDLER ( svi318_ppi_port_b_r )
{
	int row;

	row = ppi8255_peek (0, 2) & 0x0f;
	if (row <= 10)
	{
		if (row == 6)
			return readinputport(row) & readinputport(14);
		else
			return readinputport(row);
	}
	return 0xff;
}

/*
 PPI Port C Output (address 97H)
 Bit Name   Description
  1  KB0    Keyboard, Line select 0
  2  KB1    Keyboard, Line select 1
  3  KB2    Keyboard, Line select 2
  4  KB3    Keyboard, Line select 3
  5  CASON  Cassette, Motor relay control (0=on, 1=off)
  6  CASW   Cassette, Write data
  7  CASAUD Cassette, Audio out (pulse)
  8  SOUND  Keyboard, Click sound bit (pulse)
*/

static WRITE8_HANDLER ( svi318_ppi_port_c_w )
{
	int val;

	/* key click */
	val = (data & 0x80) ? 0x3e : 0;
	val += (data & 0x40) ? 0x3e : 0;
	DAC_signed_data_w (0, val);

	/* cassette motor on/off */
	if (svi318_cassette_present(0))
	{
		cassette_change_state(
			image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x10) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED,
			CASSETTE_MOTOR_DISABLED);
	}

	/* cassette signal write */
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x20) ? -1.0 : +1.0);
}

static const ppi8255_interface svi318_ppi8255_interface =
{
	1,
	{svi318_ppi_port_a_r},
	{svi318_ppi_port_b_r},
	{NULL},
	{NULL},
	{NULL},
	{svi318_ppi_port_c_w}
};

READ8_HANDLER( svi318_ppi_r )
{
	return ppi8255_0_r (offset);
}

WRITE8_HANDLER( svi318_ppi_w )
{
	ppi8255_0_w (offset + 2, data);
}

/* Printer port */

WRITE8_HANDLER( svi318_printer_w )
{
	if (!offset)
		svi.prn_data = data;
	else
	{
		if ( (svi.prn_strobe & 1) && !(data & 1) )
			printer_output(image_from_devtype_and_index(IO_PRINTER, 0), svi.prn_data);

		svi.prn_strobe = data;
	}
}

READ8_HANDLER( svi318_printer_r )
{
	if (printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0) )
		return 0xfe;

	return 0xff;
}

/* PSG */

/*
 PSG Port A Input
 Bit Name   Description
  1  FWD1   Joystick 1, Forward
  2  BACK1  Joystick 1, Back
  3  LEFT1  Joystick 1, Left
  4  RIGHT1 Joystick 1, Right
  5  FWD2   Joystick 2, Forward
  6  BACK2  Joystick 2, Back
  7  LEFT2  Joystick 2, Left
  8  RIGHT2 Joystick 2, Right
*/

READ8_HANDLER( svi318_psg_port_a_r )
{
	return readinputport (11);
}

/*
 PSG Port B Output
 Bit Name    Description
  1  /CART   Memory bank 11, ROM 0000-7FFF (Cartridge /CCS1, /CCS2)
  2  /BK21   Memory bank 21, RAM 0000-7FFF
  3  /BK22   Memory bank 22, RAM 8000-FFFF
  4  /BK31   Memory bank 31, RAM 0000-7FFF
  5  /BK32   Memory bank 32, RAM 8000-7FFF
  6  CAPS    Caps-Lock diod
  7  /ROMEN0 Memory bank 12, ROM 8000-BFFF* (Cartridge /CCS3)
  8  /ROMEN1 Memory bank 12, ROM C000-FFFF* (Cartridge /CCS4)

 * The /CART signal must be active for any effect and all banks
 with RAM are disabled.
*/

WRITE8_HANDLER( svi318_psg_port_b_w )
{
	if ( (svi.bank_switch ^ data) & 0x20)
		set_led_status (0, !(data & 0x20) );

	svi.bank_switch = data;
	svi318_set_banks ();
}

/* Disk drives  */

typedef struct
{
	UINT8 driveselect;
	UINT8 irq_drq;
	UINT8 heads[2];
} SVI318_FDC_STRUCT;

static SVI318_FDC_STRUCT svi318_fdc;

static void svi_fdc_callback(wd17xx_state_t state, void *param)
{
	switch( state )
	{
	case WD17XX_IRQ_CLR:
		svi318_fdc.irq_drq &= ~0x80;
		break;
	case WD17XX_IRQ_SET:
		svi318_fdc.irq_drq |= 0x80;
		break;
	case WD17XX_DRQ_CLR:
		svi318_fdc.irq_drq &= ~0x40;
		break;
	case WD17XX_DRQ_SET:
		svi318_fdc.irq_drq |= 0x40;
		break;
	}
}

WRITE8_HANDLER( svi318_fdc_drive_motor_w )
{
	switch (data & 3)
	{
	case 1:
		wd17xx_set_drive(0);
		svi318_fdc.driveselect = 0;
		break;
	case 2:
		wd17xx_set_drive(1);
		svi318_fdc.driveselect = 1;
		break;
	}
}

WRITE8_HANDLER( svi318_fdc_density_side_w )
{
	mess_image *image;

	wd17xx_set_density(data & 0x01 ? DEN_FM_LO:DEN_MFM_LO);

	wd17xx_set_side(data & 0x02 ? 1:0);
            
	image = image_from_devtype_and_index(IO_FLOPPY, svi318_fdc.driveselect);
	if (image_exists(image))
	{
		UINT8 sectors;
		UINT16 sectorSize;
		sectors =  data & 0x01 ? 18:17;
		sectorSize = data & 0x01 ? 128:256;
		basicdsk_set_geometry(image, 40, svi318_fdc.heads[svi318_fdc.driveselect], sectors, sectorSize, 1, 0, FALSE);
	}
}

READ8_HANDLER( svi318_fdc_irqdrq_r )
{
	return svi318_fdc.irq_drq;
}

static unsigned long svi318_calcoffset(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero)
{
	unsigned long o;

	if ((t==0) && (h==0)) 
		o = (s-first_sector_id)*128; 
	else
		o = ((t*heads+h)*17+s-first_sector_id)*256-2048; /* (17*256)-(18*128)=2048 */

	return o;
}

DEVICE_LOAD( svi318_floppy )
{
	int size;
	int id = image_index_in_device(image);

	if (!image_has_been_created(image))
	{
		size = image_length (image);

		switch (size)
		{
		case 172032: /* Single sided */
			svi318_fdc.heads[id] = 1;
			break;
		case 346112: /* Double sided */
			svi318_fdc.heads[id] = 2;
			break;
		default:
			return INIT_FAIL;
		}
	}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry(image, 40, svi318_fdc.heads[id], 17, 256, 1, 0, FALSE);
	basicdsk_set_calcoffset(image, svi318_calcoffset);

	return INIT_PASS;
}

static void svi806_crtc6845_update_row(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 ma,
									   UINT8 ra, UINT16 y, UINT8 x_count, void *param ) {
	int i;

	for( i = 0; i < x_count; i++ ) {
		int j;
		UINT8	data = svi.svi806_gfx[ svi.svi806_ram[ ( ma + i ) & 0x7FF ] * 16 + ra ];

		for( j=0; j < 8; j++ ) {
			*BITMAP_ADDR16(bitmap, y, i * 8 + j ) = TMS9928A_PALETTE_SIZE + ( ( data & 0x80 ) ? 1 : 0 );
			data = data << 1;
		}
	}
}

static const crtc6845_interface svi806_crtc6845_interface = {
	1,
	3579545 /*?*/,
	8 /*?*/,
	NULL,
	svi806_crtc6845_update_row,
	NULL,
	NULL
};

static void svi806_set_crtc_register(UINT8 reg, UINT8 data) {
	crtc6845_0_address_w(0, reg);
	crtc6845_0_register_w(0, data);
}

/* 80 column card init */
static void svi318_80col_init(void)
{
	/* 2K RAM, but allocating 4KB to make banking easier */
	/* The upper 2KB will be set to FFs and will never be written to */
	svi.svi806_ram = new_memory_region( Machine, REGION_GFX2, 0x1000, 0 );
	memset( svi.svi806_ram, 0x00, 0x800 );
	memset( svi.svi806_ram + 0x800, 0xFF, 0x800 );
	svi.svi806_gfx = memory_region(REGION_GFX1);
	/* initialise 6845 */
	crtc6845_config( 0, &svi806_crtc6845_interface);
	/* set some default values for the 6845 controller */
	svi806_set_crtc_register(  0, 109 );
	svi806_set_crtc_register(  1,  80 );
	svi806_set_crtc_register(  2,  89 );
	svi806_set_crtc_register(  3,  12 );
	svi806_set_crtc_register(  4,  31 );
	svi806_set_crtc_register(  5,   2 );
	svi806_set_crtc_register(  6,  24 );
	svi806_set_crtc_register(  7,  26 );
	svi806_set_crtc_register(  8,   0 );
	svi806_set_crtc_register(  9,   7 );
	svi806_set_crtc_register( 10,  96 );
	svi806_set_crtc_register( 11,   7 );
	svi806_set_crtc_register( 12,   0 );
	svi806_set_crtc_register( 13,   0 );
	svi806_set_crtc_register( 14,   0 );
	svi806_set_crtc_register( 15,   0 );
}

READ8_HANDLER( svi806_r )
{
	return crtc6845_0_register_r( offset );
}

WRITE8_HANDLER( svi806_w )
{
	switch( offset ) {
	case 0:
		crtc6845_0_address_w( offset, data );
		break;
	case 1:
		crtc6845_0_register_w( offset, data );
		break;
	}
}

WRITE8_HANDLER( svi806_ram_enable_w )
{
	svi.svi806_ram_enabled = ( data & 0x01 );
	svi318_set_banks();
}

VIDEO_UPDATE( svi328_806 )
{
	if ( screen == 0 )
		video_update_tms9928a(machine, screen, bitmap, cliprect);
	if ( screen == 1 )
		video_update_crtc6845(machine, screen, bitmap, cliprect);
	return 0;
}

MACHINE_RESET( svi328_806 )
{
	machine_reset_svi318(machine);
	svi318_80col_init();
	svi.svi806_present = 1;
	svi318_set_banks();

	/* Set SVI-806 80 column card palette */
	palette_set_color_rgb( machine, TMS9928A_PALETTE_SIZE, 0, 0, 0 );		/* Monochrome black */
	palette_set_color_rgb( machine, TMS9928A_PALETTE_SIZE+1, 0, 224, 0 );	/* Monochrome green */
}

/* Init functions */

void svi318_vdp_interrupt(int i)
{
	cpunum_set_input_line(0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

DRIVER_INIT( svi318 )
{
	int i, n;

	/* z80 stuff */
	static int z80_cycle_table[] =
	{
		Z80_TABLE_op, Z80_TABLE_cb, Z80_TABLE_xy,
        	Z80_TABLE_ed, Z80_TABLE_xycb, Z80_TABLE_ex
	};

	memset(&svi, 0, sizeof (svi) );

	if ( ! strcmp( machine->gamedrv->name, "svi318" ) || ! strcmp( machine->gamedrv->name, "svi318n" ) ) {
		svi.svi318 = 1;
	}

	cpunum_set_input_line_vector (0, 0, 0xff);
	ppi8255_init (&svi318_ppi8255_interface);

	/* memory */
	svi.empty_bank = auto_malloc (0x8000);
	memset (svi.empty_bank, 0xff, 0x8000);

	/* adjust z80 cycles for the M1 wait state */
	for (i = 0; i < sizeof(z80_cycle_table) / sizeof(z80_cycle_table[0]); i++)
	{
		UINT8 *table = auto_malloc (0x100);
		const UINT8 *old_table;

		old_table = cpunum_get_info_ptr (0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i]);
		memcpy (table, old_table, 0x100);

		if (z80_cycle_table[i] == Z80_TABLE_ex)
		{
			table[0x66]++; /* NMI overhead (not used) */
			table[0xff]++; /* INT overhead */
		}
		else
		{
			for (n=0; n<256; n++)
			{
				if (z80_cycle_table[i] == Z80_TABLE_op)
				{
					table[n]++;
				}
				else {
					table[n] += 2;
				}
			}
		}
		cpunum_set_info_ptr(0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i], (void*)table);
	}

	/* floppy */
	wd17xx_init(WD_TYPE_179X, svi_fdc_callback, NULL);

	/* serial */
	uart8250_init(0, svi318_uart8250_interface);
}

static const TMS9928a_interface svi318_tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	15,
	15,
	svi318_vdp_interrupt
};

static const TMS9928a_interface svi318_tms9929a_interface =
{
	TMS9929A,
	0x4000,
	13,
	13,
	svi318_vdp_interrupt
};

MACHINE_START( svi318_ntsc )
{
	TMS9928A_configure(&svi318_tms9928a_interface);
}

MACHINE_START( svi318_pal )
{
	TMS9928A_configure(&svi318_tms9929a_interface);
}

MACHINE_RESET( svi318 )
{
	/* video stuff */
	TMS9928A_reset();

	/* PPI */
	ppi8255_0_w(3, 0x92);

	svi.bank_switch = 0xff;
	svi318_set_banks();

	wd17xx_reset();

	uart8250_reset(0);
}

INTERRUPT_GEN( svi318_interrupt )
{
	int set;

	set = readinputport (13);
	TMS9928A_set_spriteslimit (set & 0x20);
	TMS9928A_interrupt();
}

/* Memory */

WRITE8_HANDLER( svi318_writemem1 )
{
	if ( svi.bank1_read_only )
		return;

	svi.bank1_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem2 )
{
	if ( svi.bank2_read_only)
		return;

	svi.bank2_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem3 )
{
	if ( svi.bank3_read_only)
		return;

	svi.bank3_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem4 )
{
	if ( svi.svi806_ram_enabled ) {
		if ( offset < 0x800 ) {
			svi.svi806_ram[ offset ] = data;
		}
	} else {
		if ( svi.bank3_read_only )
			return;

		svi.bank3_ptr[ 0x3000 + offset] = data;
	}
}

static void svi318_set_banks ()
{
	const UINT8 v = svi.bank_switch;

	svi.bank1 = ( v & 1 ) ? ( ( v & 2 ) ? ( ( v & 8 ) ? SVI_INTERNAL : SVI_EXPRAM3 ) : SVI_EXPRAM2 ) : SVI_CART;
	svi.bank2 = ( v & 4 ) ? ( ( v & 16 ) ? SVI_INTERNAL : SVI_EXPRAM3 ) : SVI_EXPRAM2;

	svi.bank1_ptr = svi.empty_bank;
	svi.bank1_read_only = 1;

	switch( svi.bank1 ) {
	case SVI_INTERNAL:
		svi.bank1_ptr = memory_region(REGION_CPU1);
		break;
	case SVI_CART:
		if ( pcart ) {
			svi.bank1_ptr = pcart;
		}
		break;
	case SVI_EXPRAM2:
		if ( mess_ram_size >= 64 * 1024 ) {
			svi.bank1_ptr = mess_ram + mess_ram_size - 64 * 1024;
			svi.bank1_read_only = 0;
		}
		break;
	case SVI_EXPRAM3:
		if ( mess_ram_size > 128 * 1024 ) {
			svi.bank1_ptr = mess_ram + mess_ram_size - 128 * 1024;
			svi.bank1_read_only = 0;
		}
		break;
	}

	svi.bank2_ptr = svi.bank3_ptr = svi.empty_bank;
	svi.bank2_read_only = svi.bank3_read_only = 1;

	switch( svi.bank2 ) {
	case SVI_INTERNAL:
		if ( mess_ram_size == 16 * 1024 ) {
			svi.bank3_ptr = mess_ram;
			svi.bank3_read_only = 0;
		} else {
			svi.bank2_ptr = mess_ram;
			svi.bank2_read_only = 0;
			svi.bank3_ptr = mess_ram + 0x4000;
			svi.bank3_read_only = 0;
		}
		break;
	case SVI_EXPRAM2:
		if ( mess_ram_size > 64 * 1024 ) {
			svi.bank2_ptr = mess_ram + mess_ram_size - 64 * 1024 + 32 * 1024;
			svi.bank2_read_only = 0;
			svi.bank3_ptr = mess_ram + mess_ram_size - 64 * 1024 + 48 * 1024;
			svi.bank3_read_only = 0;
		}
		break;
	case SVI_EXPRAM3:
		if ( mess_ram_size > 128 * 1024 ) {
			svi.bank2_ptr = mess_ram + mess_ram_size - 128 * 1024 + 32 * 1024;
			svi.bank2_read_only = 0;
			svi.bank3_ptr = mess_ram + mess_ram_size - 128 * 1024 + 48 * 1024;
			svi.bank3_read_only = 0;
		}
		break;
	}

	/* Check for special CART based banking */
	if ( svi.bank1 == SVI_CART && ( v & 0xc0 ) != 0xc0 ) {
		svi.bank2_ptr = svi.empty_bank;
		svi.bank2_read_only = 1;
		svi.bank3_ptr = svi.empty_bank;
		svi.bank3_read_only = 1;
		if ( pcart && ! ( v & 0x80 ) ) {
			svi.bank3_ptr = pcart + 0x4000;
		}
		if ( pcart && ! ( v & 0x40 ) ) {
			svi.bank2_ptr = pcart;
		}
	}

	memory_set_bankptr( 1, svi.bank1_ptr );
	memory_set_bankptr( 2, svi.bank2_ptr );
	memory_set_bankptr( 3, svi.bank3_ptr );

	/* SVI-806 80 column card specific banking */
	if ( svi.svi806_present ) {
		if ( svi.svi806_ram_enabled ) {
			memory_set_bankptr( 4, svi.svi806_ram );
		} else {
			memory_set_bankptr( 4, svi.bank3_ptr + 0x3000 );
		}
	}
}

/* Cassette */

int svi318_cassette_present(int id)
{
	return image_exists(image_from_devtype_and_index(IO_CASSETTE, id));
}
