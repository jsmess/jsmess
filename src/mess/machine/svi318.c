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
#include "video/mc6845.h"
#include "includes/svi318.h"
#include "cpu/z80/z80.h"
#include "video/tms9928a.h"
#include "machine/8255ppi.h"
#include "machine/ins8250.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "formats/svi_cas.h"
#include "sound/dac.h"
#include "sound/ay8910.h"

enum {
	SVI_INTERNAL	= 0,
	SVI_CART    	= 1,
	SVI_EXPRAM2 	= 2,
	SVI_EXPRAM3 	= 3
};

typedef struct {
	/* general */
	UINT8	svi318;		/* Are we dealing with an SVI-318 or a SVI-328 model. 0 = 328, 1 = 318 */
	/* memory */
	UINT8	*empty_bank;
	UINT8	bank_switch;
	UINT8	bankLow;
	UINT8	bankHigh1;
	UINT8	*bankLow_ptr;
	UINT8	bankLow_read_only;
	UINT8	*bankHigh1_ptr;
	UINT8	bankHigh1_read_only;
	UINT8	*bankHigh2_ptr;
	UINT8	bankHigh2_read_only;
	/* keyboard */
	UINT8	keyboard_row;
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

static void svi318_set_banks(running_machine *machine);


/* Serial ports */

static INS8250_INTERRUPT( svi318_ins8250_interrupt )
{
	if (svi.bankLow != SVI_CART) {
		cpunum_set_input_line(device->machine, 0, 0, (state ? HOLD_LINE : CLEAR_LINE));
	}
}

static INS8250_REFRESH_CONNECT( svi318_com_refresh_connected )
{
	/* Motorola MC14412 modem */
	ins8250_handshake_in(device, UART8250_HANDSHAKE_IN_CTS|UART8250_HANDSHAKE_IN_DSR|UART8250_INPUTS_RING_INDICATOR|UART8250_INPUTS_DATA_CARRIER_DETECT);
}

const ins8250_interface svi318_ins8250_interface[2]=
{
	{
		1000000,
		svi318_ins8250_interrupt,
		NULL,
		NULL,
		svi318_com_refresh_connected
	},
	{
		3072000,
		svi318_ins8250_interrupt,
		NULL,
		NULL,
		NULL
	}
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

DEVICE_START( svi318_cart )
{
	pcart = NULL;
	pcart_rom_size = 0;
}

DEVICE_IMAGE_LOAD( svi318_cart )
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

DEVICE_IMAGE_UNLOAD( svi318_cart )
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
	data |= input_port_read(machine, "BUTTONS") & 0x30;

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
	char port[7];

	row = svi.keyboard_row;
	if (row <= 10)
	{
		sprintf(port, "LINE%d", row);
		return input_port_read(machine, port);
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

	svi.keyboard_row = data & 0x0F;
}

const ppi8255_interface svi318_ppi8255_interface =
{
	svi318_ppi_port_a_r,
	svi318_ppi_port_b_r,
	NULL,
	NULL,
	NULL,
	svi318_ppi_port_c_w
};

READ8_DEVICE_HANDLER( svi318_ppi_r )
{
	return ppi8255_r(device, offset);
}

WRITE8_DEVICE_HANDLER( svi318_ppi_w )
{
	ppi8255_w(device, offset + 2, data);
}

/* Printer port */

static const device_config *printer_device(running_machine *machine)
{
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, "printer");
}

static WRITE8_HANDLER( svi318_printer_w )
{
	if (!offset)
		svi.prn_data = data;
	else
	{
		if ( (svi.prn_strobe & 1) && !(data & 1) )
			printer_output(printer_device(machine), svi.prn_data);

		svi.prn_strobe = data;
	}
}

static READ8_HANDLER( svi318_printer_r )
{
	return printer_is_ready(printer_device(machine)) ? 0xfe:0xff;		/* 0xfe if printer is ready to work */
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
	return input_port_read(machine, "JOYSTICKS");
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
	svi318_set_banks(machine);
}

/* Disk drives  */

typedef struct
{
	UINT8 driveselect;
	UINT8 irq_drq;
	UINT8 heads[2];
} SVI318_FDC_STRUCT;

static SVI318_FDC_STRUCT svi318_fdc;

static void svi_fdc_callback(running_machine *machine, wd17xx_state_t state, void *param)
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

static WRITE8_HANDLER( svi318_fdc_drive_motor_w )
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

static WRITE8_HANDLER( svi318_fdc_density_side_w )
{
	const device_config *image;

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

static READ8_HANDLER( svi318_fdc_irqdrq_r )
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

DEVICE_IMAGE_LOAD( svi318_floppy )
{
	int size;
	int dsktype;
	int id = image_index_in_device(image);

	if (!image_has_been_created(image))
	{
		size = image_length (image);

		switch (size)
		{
		case 172032:	/* SVI-328 SSDD */
			svi318_fdc.heads[id] = 1;
			dsktype = 0;
			break;
		case 346112:	/* SVI-328 DSDD */
			svi318_fdc.heads[id] = 2;
			dsktype = 0;
			break;
		case 348160:	/* SVI-728 DSDD CP/M */
			svi318_fdc.heads[id] = 2;
			dsktype = 1;
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

	if (dsktype == 0) basicdsk_set_calcoffset(image, svi318_calcoffset);

	return INIT_PASS;
}

MC6845_UPDATE_ROW( svi806_crtc6845_update_row )
{
	int i;

	for( i = 0; i < x_count; i++ )
	{
		int j;
		UINT8	data = svi.svi806_gfx[ svi.svi806_ram[ ( ma + i ) & 0x7FF ] * 16 + ra ];

		if ( i == cursor_x ) {
			data = 0xFF;
		}

		for( j=0; j < 8; j++ )
		{
			*BITMAP_ADDR16(bitmap, y, i * 8 + j ) = TMS9928A_PALETTE_SIZE + ( ( data & 0x80 ) ? 1 : 0 );
			data = data << 1;
		}
	}
}

static void svi806_set_crtc_register(UINT8 reg, UINT8 data) {	
	/* 
		NPW 9-Mar-2008 - This is bad; commenting out

		mc6845_address_w(mc6845, 0, reg);
		mc6845_register_w(mc6845, 0, data);
	*/
}

static TIMER_CALLBACK(svi318_80col_init_registers) {
	cpuintrf_push_context( 0 );
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
	cpuintrf_pop_context();
}

/* 80 column card init */
static void svi318_80col_init(running_machine *machine)
{
	/* 2K RAM, but allocating 4KB to make banking easier */
	/* The upper 2KB will be set to FFs and will never be written to */
	svi.svi806_ram = new_memory_region( machine, REGION_GFX2, 0x1000, 0 );
	memset( svi.svi806_ram, 0x00, 0x800 );
	memset( svi.svi806_ram + 0x800, 0xFF, 0x800 );
	svi.svi806_gfx = memory_region(REGION_GFX1);

	timer_set( attotime_zero, NULL, 0, svi318_80col_init_registers );
}

static WRITE8_HANDLER( svi806_ram_enable_w )
{
	svi.svi806_ram_enabled = ( data & 0x01 );
	svi318_set_banks(machine);
}

VIDEO_START( svi328_806 )
{
	VIDEO_START_CALL(tms9928a);
}

VIDEO_UPDATE( svi328_806 )
{
	if (!strcmp(screen->tag, "main"))
	{
		VIDEO_UPDATE_CALL(tms9928a);
	}
	else if (!strcmp(screen->tag, "svi806"))
	{
		const device_config *mc6845 = device_list_find_by_tag(screen->machine->config->devicelist, MC6845, "crtc");
		mc6845_update(mc6845, bitmap, cliprect);
	}
	else
	{
		fatalerror("Unknown screen %s\n", screen->tag);
	}
	return 0;
}

MACHINE_RESET( svi328_806 )
{
	MACHINE_RESET_CALL(svi318);

	svi318_80col_init(machine);
	svi.svi806_present = 1;
	svi318_set_banks(machine);

	/* Set SVI-806 80 column card palette */
	palette_set_color_rgb( machine, TMS9928A_PALETTE_SIZE, 0, 0, 0 );		/* Monochrome black */
	palette_set_color_rgb( machine, TMS9928A_PALETTE_SIZE+1, 0, 224, 0 );	/* Monochrome green */
}

/* Init functions */

void svi318_vdp_interrupt(running_machine *machine, int i)
{
	cpunum_set_input_line(machine, 0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

DRIVER_INIT( svi318 )
{
	int i, n;

	/* z80 stuff */
	static const int z80_cycle_table[] =
	{
		Z80_TABLE_op, Z80_TABLE_cb, Z80_TABLE_xy,
        	Z80_TABLE_ed, Z80_TABLE_xycb, Z80_TABLE_ex
	};

	memset(&svi, 0, sizeof (svi) );

	if ( ! strcmp( machine->gamedrv->name, "svi318" ) || ! strcmp( machine->gamedrv->name, "svi318n" ) ) {
		svi.svi318 = 1;
	}

	cpunum_set_input_line_vector (0, 0, 0xff);

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
	wd17xx_init(machine, WD_TYPE_179X, svi_fdc_callback, NULL);
}

MACHINE_START( svi318_pal )
{
	TMS9928A_configure(&svi318_tms9929a_interface);
	wd17xx_init(machine, WD_TYPE_179X, svi_fdc_callback, NULL);
}

MACHINE_RESET( svi318 )
{
	/* video stuff */
	TMS9928A_reset();

	svi.bank_switch = 0xff;
	svi318_set_banks(machine);

	wd17xx_reset();
}

INTERRUPT_GEN( svi318_interrupt )
{
	int set;

	set = input_port_read(machine, "CONFIG");
	TMS9928A_set_spriteslimit (set & 0x20);
	TMS9928A_interrupt(machine);
}

/* Memory */

WRITE8_HANDLER( svi318_writemem1 )
{
	if ( svi.bankLow_read_only )
		return;

	svi.bankLow_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem2 )
{
	if ( svi.bankHigh1_read_only)
		return;

	svi.bankHigh1_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem3 )
{
	if ( svi.bankHigh2_read_only)
		return;

	svi.bankHigh2_ptr[offset] = data;
}

WRITE8_HANDLER( svi318_writemem4 )
{
	if ( svi.svi806_ram_enabled ) {
		if ( offset < 0x800 ) {
			svi.svi806_ram[ offset ] = data;
		}
	} else {
		if ( svi.bankHigh2_read_only )
			return;

		svi.bankHigh2_ptr[ 0x3000 + offset] = data;
	}
}

static void svi318_set_banks(running_machine *machine)
{
	const UINT8 v = svi.bank_switch;

	svi.bankLow = ( v & 1 ) ? ( ( v & 2 ) ? ( ( v & 8 ) ? SVI_INTERNAL : SVI_EXPRAM3 ) : SVI_EXPRAM2 ) : SVI_CART;
	svi.bankHigh1 = ( v & 4 ) ? ( ( v & 16 ) ? SVI_INTERNAL : SVI_EXPRAM3 ) : SVI_EXPRAM2;

	svi.bankLow_ptr = svi.empty_bank;
	svi.bankLow_read_only = 1;

	switch( svi.bankLow ) {
	case SVI_INTERNAL:
		svi.bankLow_ptr = memory_region(REGION_CPU1);
		break;
	case SVI_CART:
		if ( pcart ) {
			svi.bankLow_ptr = pcart;
		}
		break;
	case SVI_EXPRAM2:
		if ( mess_ram_size >= 64 * 1024 ) {
			svi.bankLow_ptr = mess_ram + mess_ram_size - 64 * 1024;
			svi.bankLow_read_only = 0;
		}
		break;
	case SVI_EXPRAM3:
		if ( mess_ram_size > 128 * 1024 ) {
			svi.bankLow_ptr = mess_ram + mess_ram_size - 128 * 1024;
			svi.bankLow_read_only = 0;
		}
		break;
	}

	svi.bankHigh1_ptr = svi.bankHigh2_ptr = svi.empty_bank;
	svi.bankHigh1_read_only = svi.bankHigh2_read_only = 1;

	switch( svi.bankHigh1 ) {
	case SVI_INTERNAL:
		if ( mess_ram_size == 16 * 1024 ) {
			svi.bankHigh2_ptr = mess_ram;
			svi.bankHigh2_read_only = 0;
		} else {
			svi.bankHigh1_ptr = mess_ram;
			svi.bankHigh1_read_only = 0;
			svi.bankHigh2_ptr = mess_ram + 0x4000;
			svi.bankHigh2_read_only = 0;
		}
		break;
	case SVI_EXPRAM2:
		if ( mess_ram_size > 64 * 1024 ) {
			svi.bankHigh1_ptr = mess_ram + mess_ram_size - 64 * 1024 + 32 * 1024;
			svi.bankHigh1_read_only = 0;
			svi.bankHigh2_ptr = mess_ram + mess_ram_size - 64 * 1024 + 48 * 1024;
			svi.bankHigh2_read_only = 0;
		}
		break;
	case SVI_EXPRAM3:
		if ( mess_ram_size > 128 * 1024 ) {
			svi.bankHigh1_ptr = mess_ram + mess_ram_size - 128 * 1024 + 32 * 1024;
			svi.bankHigh1_read_only = 0;
			svi.bankHigh2_ptr = mess_ram + mess_ram_size - 128 * 1024 + 48 * 1024;
			svi.bankHigh2_read_only = 0;
		}
		break;
	}

	/* Check for special CART based banking */
	if ( svi.bankLow == SVI_CART && ( v & 0xc0 ) != 0xc0 ) {
		svi.bankHigh1_ptr = svi.empty_bank;
		svi.bankHigh1_read_only = 1;
		svi.bankHigh2_ptr = svi.empty_bank;
		svi.bankHigh2_read_only = 1;
		if ( pcart && ! ( v & 0x80 ) ) {
			svi.bankHigh2_ptr = pcart + 0x4000;
		}
		if ( pcart && ! ( v & 0x40 ) ) {
			svi.bankHigh1_ptr = pcart;
		}
	}

	memory_set_bankptr( 1, svi.bankLow_ptr );
	memory_set_bankptr( 2, svi.bankHigh1_ptr );
	memory_set_bankptr( 3, svi.bankHigh2_ptr );

	/* SVI-806 80 column card specific banking */
	if ( svi.svi806_present ) {
		if ( svi.svi806_ram_enabled ) {
			memory_set_bankptr( 4, svi.svi806_ram );
		} else {
			memory_set_bankptr( 4, svi.bankHigh2_ptr + 0x3000 );
		}
	}
}

/* Cassette */

int svi318_cassette_present(int id)
{
	return image_exists(image_from_devtype_and_index(IO_CASSETTE, id));
}

/* External I/O */

READ8_HANDLER( svi318_io_ext_r )
{
	UINT8 data = 0xff;

	if (svi.bankLow == SVI_CART) {
		return 0xff;
	}

	switch( offset ) {
	case 0x12:
		data = svi318_printer_r(machine, 0);
		break;

	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		data = ins8250_r(device_list_find_by_tag( machine->config->devicelist, INS8250, "ins8250_0" ), offset & 7);
		break;

	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		data = ins8250_r(device_list_find_by_tag( machine->config->devicelist, INS8250, "ins8250_1" ), offset & 7);
		break;

	case 0x30:
		data = wd17xx_status_r(machine, 0);
		break;
	case 0x31:
		data = wd17xx_track_r(machine, 0);
		break;
	case 0x32:
		data = wd17xx_sector_r(machine, 0);
		break;
	case 0x33:
		data = wd17xx_data_r(machine, 0);
		break;
	case 0x34:
		data = svi318_fdc_irqdrq_r(machine, 0);
		break;
	case 0x51: {
		device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
		data = mc6845_register_r(devconf, 0);
		}
		break;
	}

	return data;
}

WRITE8_HANDLER( svi318_io_ext_w )
{
	if (svi.bankLow == SVI_CART) {
		return;
	}

	switch( offset ) {
	case 0x10:
	case 0x11:
		svi318_printer_w(machine, offset & 1, data);
		break;

	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		ins8250_w(device_list_find_by_tag( machine->config->devicelist, INS8250, "ins8250_0" ), offset & 7, data);
		break;

	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		ins8250_w(device_list_find_by_tag( machine->config->devicelist, INS8250, "ins8250_1" ), offset & 7, data);
		break;

	case 0x30:
		wd17xx_command_w(machine, 0, data);
		break;
	case 0x31:
		wd17xx_track_w(machine, 0, data);
		break;
	case 0x32:
		wd17xx_sector_w(machine, 0, data);
		break;
	case 0x33:
		wd17xx_data_w(machine, 0, data);
		break;
	case 0x34:
		svi318_fdc_drive_motor_w(machine, 0, data);
		break;
	case 0x38:
		svi318_fdc_density_side_w(machine, 0, data);
		break;

	case 0x50: {
		device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
		mc6845_address_w(devconf, 0, data);
		}
		break;
	case 0x51: {
		device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
		mc6845_register_w(devconf, 0, data);
		}
		break;

	case 0x58:
		svi806_ram_enable_w(machine, 0, data);
		break;
	}
}
