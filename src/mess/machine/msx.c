/*
 * msx.c: MSX emulation
 *
 * Copyright (C) 2004 Sean Young
 *
 * Todo:
 *
 * - fix mouse support
 * - cassette support doesn't work
 * - Ensure changing cartridge after boot works
 * - wd2793, nms8255
 */

#include "driver.h"
#include "includes/msx_slot.h"
#include "includes/msx.h"
#include "video/generic.h"
#include "machine/8255ppi.h"
#include "includes/tc8521.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "video/tms9928a.h"
#include "cpu/z80/z80.h"
#include "video/v9938.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "utils.h"
#include "image.h"
#include "osdepend.h"
#include "sound/ay8910.h"
#include "sound/2413intf.h"
#include "sound/dac.h"

#define VERBOSE 0

MSX msx1;
static slot_state *cart_state[MSX_MAX_CARTS];

static int msx_probe_type (UINT8* pmem, int size)
{
	int kon4, kon5, asc8, asc16, i;

	if (size <= 0x10000) return 0;

	if ( (pmem[0x10] == 'Y') && (pmem[0x11] == 'Z') && (size > 0x18000) )
		return 6;

	kon4 = kon5 = asc8 = asc16 = 0;

	for (i=0;i<size-3;i++)
	{
		if (pmem[i] == 0x32 && pmem[i+1] == 0)
		{
			switch (pmem[i+2]) {
			case 0x60:
			case 0x70:
				asc16++;
				asc8++;
				break;
			case 0x68:
			case 0x78:
				asc8++;
				asc16--;
			}

			switch (pmem[i+2]) {
			case 0x60:
			case 0x80:
			case 0xa0:
				kon4++;
				break;
			case 0x50:
			case 0x70:
			case 0x90:
			case 0xb0:
				kon5++;
			}
		}
	}

	if (MAX (kon4, kon5) > MAX (asc8, asc16) )
		return (kon5 > kon4) ? 2 : 3;
	else
		return (asc8 > asc16) ? 4 : 5;
}

DEVICE_LOAD (msx_cart)
{
	int size;
	int size_aligned;
	UINT8 *mem;
	int type;
	const char *extra;
	char *sramfile;
	slot_state *state;
	int id;

	id = image_index_in_device (image);

	size = image_length (image);
	if (size < 0x2000) {
		logerror ("cart #%d: error: file is smaller than 2kb, too small "
				  "to be true!\n", id);
		return INIT_FAIL;
	}

	/* allocate memory and load */
	size_aligned = 0x2000;
	while (size_aligned < size) {
		size_aligned *= 2;
	}
	mem = image_malloc (image, size_aligned);
	if (!mem) {
		logerror ("cart #%d: error: failed to allocate memory for cartridge\n", 
						id);
		return INIT_FAIL;
	}
	if (size < size_aligned) {
		memset (mem, 0xff, size_aligned);
	}
	if (image_fread(image, mem, size) != size) {
		logerror ("cart #%d: %s: can't read full %d bytes\n", 
						id, image_filename (image), size);
		return INIT_FAIL;
	}

	/* see if msx.crc will tell us more */
	extra = image_extrainfo (image);
	if (!extra) {
		logerror("cart #%d: warning: no information in crc file\n", id);
		type = -1;
	}
	else if ((1 != sscanf (extra, "%d", &type) ) || 
			type < 0 || type > SLOT_LAST_CARTRIDGE_TYPE) {
		logerror("cart #%d: warning: information in crc file not valid\n", id);
		type = -1;
	}
	else {
		logerror ("cart #%d: info: cart extra info: '%s' = %s\n", id, extra,
						msx_slot_list[type].name);
	}

	/* if not, attempt autodetection */
	if (type < 0) {
		type = msx_probe_type (mem, size);

		if (mem[0] != 'A' || mem[1] != 'B') {
			logerror("cart #%d: %s: May not be a valid ROM file\n",
							id, image_filename (image));
		}

		logerror("cart #%d: Probed cartridge mapper %d/%s\n", id,
						type, msx_slot_list[type].name);
	}

	/* mapper type 0 always needs 64kB */
	if (!type && size_aligned != 0x10000)
	{
		size_aligned = 0x10000;
		mem = image_realloc(image, mem, 0x10000);
		if (!mem) {
			logerror ("cart #%d: error: cannot allocate memory\n", id);
			return INIT_FAIL;
		}
		
		if (size < 0x10000) {
			memset (mem + size, 0xff, 0x10000 - size);
		}
		if (size > 0x10000) {
			logerror ("cart #%d: warning: rom truncated to 64kb due to "
					  "mapperless type (possibly detected)\n", id);

			size = 0x10000;
		}
	}

	/* mapper type 0 (ROM) might need moving around a bit */
	if (!type) {
		int i, page = 1;
		
		/* find the correct page */
		if (mem[0] == 'A' && mem[1] == 'B') {
			for (i=2; i<=8; i += 2) {
				if (mem[i] || mem[i+1]) {
					page = mem[i+1] / 0x40;
					break;
				}
			}
		}

		if (size <= 0x4000) {
			if (page == 1 || page == 2) {
				/* copy to the respective page */
				memcpy (mem + (page * 0x4000), mem, 0x4000);
				memset (mem, 0xff, 0x4000);
			} 
			else {
				/* memory is repeated 4 times */
				page = -1;
				memcpy (mem + 0x4000, mem, 0x4000);
				memcpy (mem + 0x8000, mem, 0x4000);
				memcpy (mem + 0xc000, mem, 0x4000);
			}
		}
		else /*if (size <= 0xc000) */ {
			if (page) {
				/* shift up 16kB; custom memcpy so overlapping memory
				   isn't corrupted. ROM starts in page 1 (0x4000) */
				UINT8 *m;

				page = 1;
				i = 0xc000; m = mem + 0xffff;
				while (i--) { 
					*m = *(m - 0x4000); m--; 
				}
				memset (mem, 0xff, 0x4000);
			}
		}

		if (page) {
			logerror ("cart #%d: info: rom in page %d\n", id, page);
		}
		else {
			logerror ("cart #%d: info: rom duplicted in all pages\n", id);
		}
	}

	/* kludge */
	if (type == 0) {
		type = SLOT_ROM;
	}

	/* allocate and set slot_state for this cartridge */
	state = (slot_state*) image_malloc(image, sizeof (slot_state));
	if (!state)
	{
		logerror ("cart #%d: error: cannot allocate memory for "
				  "cartridge state\n", id);
		return INIT_FAIL;
	}
	memset (state, 0, sizeof (slot_state));

	state->type = type;
	sramfile = image_malloc(image, strlen (image_filename (image) + 1));

	if (sramfile) {
		char *ext;

		strcpy (sramfile, image_basename (image));
		ext = strrchr (sramfile, '.');
		if (ext) {
			*ext = 0;
		}
		state->sramfile = sramfile;
	}

	if (msx_slot_list[type].init (state, 0, mem, size_aligned)) {
		return INIT_FAIL;
	}
	if (msx_slot_list[type].loadsram) {
		msx_slot_list[type].loadsram (state);
	}

	msx1.cart_state[id] = cart_state[id] = state;
	msx_memory_set_carts ();

	return INIT_PASS;
}

DEVICE_UNLOAD (msx_cart)
{
	int id;
	
	id = image_index_in_device (image);
	if (msx_slot_list[msx1.cart_state[id]->type].savesram)
	{
		msx_slot_list[msx1.cart_state[id]->type].savesram (msx1.cart_state[id]);
	}
}

void msx_vdp_interrupt(int i)
{
	cpunum_set_input_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

static void msx_ch_reset_core (void)
{
	msx_memory_reset ();
	msx_memory_map_all ();
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	msx_vdp_interrupt
};

MACHINE_START( msx )
{
	TMS9928A_configure(&tms9928a_interface);
	return 0;
}

MACHINE_RESET( msx )
{
	TMS9928A_reset ();
	msx_ch_reset_core ();
}

MACHINE_RESET( msx2 )
{
	v9938_reset ();
	msx_ch_reset_core ();
}

static void msx_wd179x_int (int state);

static WRITE8_HANDLER ( msx_ppi_port_a_w );
static WRITE8_HANDLER ( msx_ppi_port_c_w );
static READ8_HANDLER (msx_ppi_port_b_r );

static ppi8255_interface msx_ppi8255_interface =
{
	1,
	{NULL}, 
	{msx_ppi_port_b_r},
	{NULL},
	{msx_ppi_port_a_w},
	{NULL}, 
	{msx_ppi_port_c_w}
};

static struct tc8521_interface tc = { NULL };

static void msx_init(void)
{
	int i, n;

	/* z80 stuff */
	static int z80_cycle_table[] =
	{
		Z80_TABLE_op, Z80_TABLE_cb, Z80_TABLE_xy,
        Z80_TABLE_ed, Z80_TABLE_xycb, Z80_TABLE_ex
	};

	memset (&msx1, 0, sizeof (MSX));
	/* LOAD_DEVICE is called before DRIVER_INIT */
	for (i=0; i<MSX_MAX_CARTS; i++) {
		msx1.cart_state[i] = cart_state[i];
	}

	wd179x_init (WD_TYPE_179X, msx_wd179x_int);
	wd179x_set_density (DEN_FM_HI);
	msx1.dsk_stat = 0x7f;

	cpunum_set_input_line_vector (0, 0, 0xff);
	ppi8255_init (&msx_ppi8255_interface);

	msx_memory_init ();

	/* adjust z80 cycles for the M1 wait state */
	for (i = 0; i < sizeof(z80_cycle_table) / sizeof(z80_cycle_table[0]); i++)
	{
		UINT8 *table = auto_malloc (0x100);
		const UINT8 *old_table;

		old_table = cpunum_get_info_ptr (0,
				CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i]);
		memcpy (table, old_table, 0x100);

		if (z80_cycle_table[i] == Z80_TABLE_ex) {
			table[0x66]++; /* NMI overhead (not used) */
			table[0xff]++; /* INT overhead */
		}
		else {
			for (n=0; n<256; n++) {
				if (z80_cycle_table[i] == Z80_TABLE_op) {
					table[n]++;
				}
				else {
					table[n] += 2;
				}
			}
		}
		cpunum_set_info_ptr (0,
					CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i],
					(void*)table);
	}
}

DRIVER_INIT( msx )
{
	msx_init();
}

DRIVER_INIT( msx2 )
{
	msx_init();
	tc8521_init (&tc);
}

INTERRUPT_GEN( msx2_interrupt )
{
	v9938_set_sprite_limit(readinputport (8) & 0x20);
	v9938_set_resolution(readinputport (8) & 0x03);
	v9938_interrupt();
}

INTERRUPT_GEN( msx_interrupt )
{
	int i;

	for (i=0;i<2;i++)
	{
		msx1.mouse[i] = readinputport (9+i);
		msx1.mouse_stat[i] = -1;
	}

	TMS9928A_set_spriteslimit (readinputport (8) & 0x20);
	TMS9928A_interrupt();
}

/*
** The I/O funtions
*/

READ8_HANDLER ( msx_psg_r )
{
	return AY8910_read_port_0_r (offset);
}

WRITE8_HANDLER ( msx_psg_w )
{
	if (offset & 0x01)
		AY8910_write_port_0_w (offset, data);
	else
		AY8910_control_port_0_w (offset, data);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

static mess_image *printer_image(void)
{
	return image_from_devtype_and_index(IO_PRINTER, 0);
}

READ8_HANDLER ( msx_psg_port_a_r )
{
	int data, inp;

	data = (cassette_input(cassette_device_image()) > 0.0038 ? 0x80 : 0);

	if ( (msx1.psg_b ^ readinputport (8) ) & 0x40)
		{
		/* game port 2 */
		inp = input_port_7_r (0) & 0x7f;
#if 0
		if ( !(inp & 0x80) )
			{
#endif
			/* joystick */
			return (inp & 0x7f) | data;
#if 0
			}
		else
			{
			/* mouse */
			data |= inp & 0x70;
			if (msx1.mouse_stat[1] < 0)
				inp = 0xf;
			else
				inp = ~(msx1.mouse[1] >> (4*msx1.mouse_stat[1]) ) & 15;

			return data | inp;
			}
#endif
		}
	else
		{
		/* game port 1 */
		inp = input_port_6_r (0) & 0x7f;
#if 0
		if ( !(inp & 0x80) )
			{
#endif
			/* joystick */
			return (inp & 0x7f) | data;
#if 0
			}
		else
			{
			/* mouse */
			data |= inp & 0x70;
			if (msx1.mouse_stat[0] < 0)
				inp = 0xf;
			else
				inp = ~(msx1.mouse[0] >> (4*msx1.mouse_stat[0]) ) & 15;

			return data | inp;
			}
#endif
		}

	return 0;
}

READ8_HANDLER ( msx_psg_port_b_r )
{
	return msx1.psg_b;
}

WRITE8_HANDLER ( msx_psg_port_a_w )
{
}

WRITE8_HANDLER ( msx_psg_port_b_w )
{
	/* Arabic or kana mode led */
	if ( (data ^ msx1.psg_b) & 0x80)
		set_led_status (2, !(data & 0x80) );

	if ( (msx1.psg_b ^ data) & 0x10)
	{
		if (++msx1.mouse_stat[0] > 3) msx1.mouse_stat[0] = -1;
	}
	if ( (msx1.psg_b ^ data) & 0x20)
	{
		if (++msx1.mouse_stat[1] > 3) msx1.mouse_stat[1] = -1;
	}

	msx1.psg_b = data;
}

WRITE8_HANDLER ( msx_printer_w )
{
	if (readinputport (8) & 0x80) {
		/* SIMPL emulation */
		if (offset == 1)
			DAC_signed_data_w (0, data);
	}
	else {

		if (offset == 1)
		{
			msx1.prn_data = data;
		}
		else
		{
			if ((msx1.prn_strobe & 2) && !(data & 2))
			{
				printer_output(printer_image(), msx1.prn_data);
			}

			msx1.prn_strobe = data;
		}
	}
}

READ8_HANDLER ( msx_printer_r )
{
	if (offset == 0 && ! (readinputport (8) & 0x80) &&
			printer_status(printer_image(), 0) )
		return 253;

	return 0xff;
}

WRITE8_HANDLER (msx_fmpac_w)
{
	if (msx1.opll_active) {

		if (offset == 1) {
			YM2413_data_port_0_w (0, data);
		}
		else {
			YM2413_register_port_0_w (0, data);
		}
	}
}

/*
** RTC functions
*/

WRITE8_HANDLER (msx_rtc_latch_w)
{
	msx1.rtc_latch = data & 15;
}

WRITE8_HANDLER (msx_rtc_reg_w)
{
	tc8521_w (msx1.rtc_latch, data);
}

READ8_HANDLER (msx_rtc_reg_r)
{
	return tc8521_r (msx1.rtc_latch);
}

NVRAM_HANDLER( msx2 )
{
	if (file) {

		if (read_or_write) {
			tc8521_save_stream (file);
		}
		else {
			tc8521_load_stream (file);
		}
	}
}

/*
From: erbo@xs4all.nl (erik de boer)

sony and philips have used (almost) the same design
and this is the memory layout
but it is not a msx standard !

WD1793 or wd2793 registers

adress

7FF8H read	status register
	  write command register
7FF9H  r/w	track register (r/o on NMS 8245 and Sony)
7FFAH  r/w	sector register (r/o on NMS 8245 and Sony)
7FFBH  r/w	data register


hardware registers

adress

7FFCH r/w  bit 0 side select
7FFDH r/w  b7>M-on , b6>in-use , b1>ds1 , b0>ds0  (all neg. logic)
7FFEH		  not used
7FFFH read b7>drq , b6>intrq

set on 7FFDH bit 2 always to 0 (some use it as disk change reset)

*/

static void msx_wd179x_int (int state)
	{
	switch (state) {
		case WD179X_IRQ_CLR: msx1.dsk_stat |= 0x40; break;
		case WD179X_IRQ_SET: msx1.dsk_stat &= ~0x40; break;
		case WD179X_DRQ_CLR: msx1.dsk_stat |= 0x80; break;
		case WD179X_DRQ_SET: msx1.dsk_stat &= ~0x80; break;
	}
}


DEVICE_LOAD( msx_floppy )
{
	int size, heads = 2;

	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 360*1024:
				heads = 1;
			case 720*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 80, heads, 9, 512, 1, 0, FALSE);

	return INIT_PASS;
}

/*
** The PPI functions
*/

static WRITE8_HANDLER ( msx_ppi_port_a_w )
{
	msx1.primary_slot = ppi8255_peek (0,0);
	if (VERBOSE)
		logerror ("write to primary slot select: %02x\n", msx1.primary_slot);
	msx_memory_map_all ();
}

static WRITE8_HANDLER ( msx_ppi_port_c_w )
{
	static int old_val = 0xff;

	/* caps lock */
	if ( (old_val ^ data) & 0x40)
		set_led_status (1, !(data & 0x40) );

	/* key click */
	if ( (old_val ^ data) & 0x80)
		DAC_signed_data_w (0, (data & 0x80 ? 0x7f : 0));

	/* cassette motor on/off */
	if ( (old_val ^ data) & 0x10)
		cassette_change_state(cassette_device_image(), 
						(data & 0x10) ? CASSETTE_MOTOR_DISABLED : 
										CASSETTE_MOTOR_ENABLED, 
						CASSETTE_MASK_MOTOR);

	/* cassette signal write */
	if ( (old_val ^ data) & 0x20)
		cassette_output(cassette_device_image(), (data & 0x20) ? -1.0 : 1.0);

	old_val = data;
}

static READ8_HANDLER( msx_ppi_port_b_r )
{
	UINT8 result = 0xff;
	int row, data;

	row = ppi8255_0_r (2) & 0x0f;
	if (row <= 10)
	{
		data = readinputport (row/2);
		if (row & 1)
			data >>= 8;
		result = data & 0xff;
	}
	return result;
}

/************************************************************************
 *
 * New memory emulation !!
 *
 ***********************************************************************/

void msx_memory_init (void)
{
	int	prim, sec, page, extent, option;
	int size = 0;
	const msx_slot_layout *layout= (msx_slot_layout*)NULL;
	const msx_slot *slot;
	const msx_driver_struct *driver;
	slot_state *st;
	UINT8 *mem = NULL;

	msx1.empty = (UINT8*)auto_malloc (0x4000);
	memset (msx1.empty, 0xff, 0x4000);

	for (prim=0; prim<4; prim++) {
		for (sec=0; sec<4; sec++) {
			for (page=0; page<4; page++) {
				msx1.all_state[prim][sec][page]= (slot_state*)NULL;
			}
		}
	}

	for (driver = msx_driver_list; driver->name[0]; driver++) {
		if (!strcmp (driver->name, Machine->gamedrv->name)) {
			layout = driver->layout;
		}
	}

	if (!layout) {
		logerror ("msx_memory_init: error: missing layout definition in "
				  "msx_driver_list\n");
		return;
	}

	for (; layout->entry != MSX_LAYOUT_LAST; layout++) {
		
		switch (layout->entry) {
		case MSX_LAYOUT_SLOT_ENTRY:
			prim = layout->slot_primary;
			sec = layout->slot_secondary;
			page = layout->slot_page;
			extent = layout->page_extent;

			if (layout->slot_secondary) {
				msx1.slot_expanded[layout->slot_primary]= TRUE;
			}

			slot = &msx_slot_list[layout->type];
			if (slot->slot_type != layout->type) {
				logerror ("internal error: msx_slot_list[%d].type != %d\n",
							slot->slot_type, slot->slot_type);
			}

			size = layout->size;
			option = layout->option;

			if (VERBOSE)
			{
				logerror ("slot %d/%d/%d-%d: type %s, size 0x%x\n",
					prim, sec, page, page + extent - 1, slot->name, size);
			}

			st = (slot_state*)NULL;
			if (layout->type == SLOT_CARTRIDGE1) {
				st = msx1.cart_state[0];
				if (!st) {
					slot = &msx_slot_list[SLOT_SOUNDCARTRIDGE];
					size = 0x20000;
				}
			}
			if (layout->type == SLOT_CARTRIDGE2) {
				st = msx1.cart_state[1];
				if (!st) {
					/* Check whether the optional FM-PAC rom is present */
					option = 0x10000;
					size = 0x10000;
					mem = memory_region(REGION_CPU1) + option;
					if (memory_region_length(REGION_CPU1) > size && mem[0] == 'A' && mem[1] == 'B') {
						slot = &msx_slot_list[SLOT_FMPAC];
					}
					else {
						slot = &msx_slot_list[SLOT_EMPTY];
					}
				}
			}

			if (!st) {
				switch (slot->mem_type) {

				case MSX_MEM_HANDLER:
				case MSX_MEM_ROM:
					mem = memory_region(REGION_CPU1) + option;
					break;
				case MSX_MEM_RAM:
					mem = NULL;
					break;
				}
				st = (slot_state*)auto_malloc (sizeof (slot_state));
				memset (st, 0, sizeof (slot_state));

				if (slot->init (st, layout->slot_page, mem, size)) {
					continue;
				}
			}

			while (extent--) {
				if (page > 3) {
					logerror ("internal error: msx_slot_layout wrong, "
						 "page + extent > 3\n");
					break;
				}
				msx1.all_state[prim][sec][page] = st;
				page++;
			}
			break;
		case MSX_LAYOUT_KANJI_ENTRY:
			msx1.kanji_mem = memory_region(REGION_CPU1) + layout->option;
			break;
		case MSX_LAYOUT_RAMIO_SET_BITS_ENTRY:
			msx1.ramio_set_bits = (UINT8)layout->option;
			break;
		}
	}
}

void msx_memory_reset (void)
{
	slot_state *state, *last_state= (slot_state*)NULL;
	int prim, sec, page;

	msx1.primary_slot = 0;

	for (prim=0; prim<4; prim++) {
		msx1.secondary_slot[prim] = 0;
		for (sec=0; sec<4; sec++) {
			for (page=0; page<4; page++) {
				state = msx1.all_state[prim][sec][page];
				if (state && state != last_state) {
					msx_slot_list[state->type].reset (state);
				}
				last_state = state;
			}
		}
	}
}

void msx_memory_set_carts (void)
{
	const msx_slot_layout *layout;
	int page;

	if (!msx1.layout) {
		return;
	}
	
	for (layout = msx1.layout; layout->entry != MSX_LAYOUT_LAST; 
					layout++) {

		if (layout->entry == MSX_LAYOUT_SLOT_ENTRY) {

			switch (layout->type) {
			case SLOT_CARTRIDGE1:
				for (page=0; page<4; page++) {
					msx1.all_state[layout->slot_primary]
								  [layout->slot_secondary]
								  [page] = msx1.cart_state[0];
				}
				break;
			case SLOT_CARTRIDGE2:
				for (page=0; page<4; page++) {
					msx1.all_state[layout->slot_primary]
								  [layout->slot_secondary]
								  [page] = msx1.cart_state[1];
				}
				break;
			}
		}
	}
}

void msx_memory_map_page (int page)
{
	int slot_primary;
	int slot_secondary;
	slot_state *state;
	const msx_slot *slot;

	slot_primary = (msx1.primary_slot >> (page * 2)) & 3;
	slot_secondary = (msx1.secondary_slot[slot_primary] >> (page * 2)) & 3;

	state = msx1.all_state[slot_primary][slot_secondary][page];
	slot = state ? &msx_slot_list[state->type] : &msx_slot_list[SLOT_EMPTY];
	msx1.state[page] = state;
	msx1.slot[page] = slot;

	if (VERBOSE)
	{
		logerror ("mapping %s in %d/%d/%d\n", slot->name, slot_primary, 
			slot_secondary, page);
	}
	slot->map (state, page);
}

void msx_memory_map_all (void)
{
	int i;

	for (i=0; i<4; i++) {
		msx_memory_map_page (i);
	}
}

WRITE8_HANDLER (msx_superloadrunner_w)
{
	msx1.superloadrunner_bank = data;
	if (msx1.slot[2]->slot_type == SLOT_SUPERLOADRUNNER) {
		msx1.slot[2]->map (msx1.state[2], 2);
	}
	msx_page0_w (-1, data);
}

WRITE8_HANDLER (msx_page0_w)
{
	offset++;

	switch (msx1.slot[0]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[0][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[0]->write (msx1.state[0], offset, data);
	}
}

WRITE8_HANDLER (msx_page1_w)
{
	switch (msx1.slot[1]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[1][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[1]->write (msx1.state[1], 0x4000 + offset, data);
	}
}

WRITE8_HANDLER (msx_page2_w)
{
	switch (msx1.slot[2]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[2][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[2]->write (msx1.state[2], 0x8000 + offset, data);
	}
}

WRITE8_HANDLER (msx_page3_w)
{
	switch (msx1.slot[3]->mem_type) {
	case MSX_MEM_RAM:
		msx1.ram_pages[3][offset] = data;
		break;
	case MSX_MEM_HANDLER:
		msx1.slot[3]->write (msx1.state[3], 0xc000 + offset, data);
	}
}

WRITE8_HANDLER (msx_sec_slot_w)
{
	int slot = msx1.primary_slot >> 6;
	if (msx1.slot_expanded[slot])
	{
		if (VERBOSE)
			logerror ("write to secondary slot %d select: %02x\n", slot, data);

		msx1.secondary_slot[slot] = data;
		msx_memory_map_all ();
	}
	else {
		msx_page3_w (0x3fff, data);
	}
}

READ8_HANDLER (msx_sec_slot_r)
{
	UINT8 result;
	int slot = msx1.primary_slot >> 6;

	if (msx1.slot_expanded[slot])
		result = ~msx1.secondary_slot[slot];
	else
		result = msx1.top_page[0x1fff];

	return result;
}

WRITE8_HANDLER (msx_ram_mapper_w)
{
	msx1.ram_mapper[offset] = data;
	if (msx1.slot[offset]->slot_type == SLOT_RAM_MM) {
		msx1.slot[offset]->map (msx1.state[offset], offset);
	}
}

READ8_HANDLER (msx_ram_mapper_r)
{
	return msx1.ram_mapper[offset] | msx1.ramio_set_bits;
}

WRITE8_HANDLER (msx_90in1_w)
{
	msx1.korean90in1_bank = data;
	if (msx1.slot[1]->slot_type == SLOT_KOREAN_90IN1)
	{
		msx1.slot[1]->map (msx1.state[1], 1);
	}
	if (msx1.slot[2]->slot_type == SLOT_KOREAN_90IN1)
	{
		msx1.slot[2]->map (msx1.state[2], 2);
	}
}

READ8_HANDLER (msx_kanji_r)
{
	UINT8 result = 0xff;

	if (msx1.kanji_mem)
	{
		int latch;
		UINT8 ret;
		
		latch = msx1.kanji_latch;
		ret = msx1.kanji_mem[latch++];

		msx1.kanji_latch &= ~0x1f;
		msx1.kanji_latch |= latch & 0x1f;

		result = ret;
	}
	return result;
}

WRITE8_HANDLER (msx_kanji_w)
{
	if (offset)
	{
		msx1.kanji_latch = 
				(msx1.kanji_latch & 0x007E0) | ((data & 0x3f) << 11);
	}
	else
	{
		msx1.kanji_latch = 
				(msx1.kanji_latch & 0x1f800) | ((data & 0x3f) << 5);
	}
}
