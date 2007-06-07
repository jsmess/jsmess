/******************************************************************************
 MC-10

  TODO
	RS232
	Printer
	Disk
 *****************************************************************************/

#include "driver.h"
#include "video/m6847.h"
#include "includes/mc10.h"
#include "devices/cassette.h"
#include "sound/dac.h"

static UINT8 mc10_bfff;
static UINT8 mc10_keyboard_strobe;

void mc10_init_machine(void)
{
	mc10_keyboard_strobe = 0xff;

	/* NPW: Taken from Juergen's MC-10 attempt that I just noticed... */
	if( readinputport(7) & 0x80 )
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MRA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MWA8_RAM);
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MWA8_NOP);
	}
	/* Install DOS ROM ? */
	if( readinputport(7) & 0x40 )
	{
		file_error filerr;
		mame_file *rom;

		filerr = mame_fopen(SEARCHPATH_IMAGE, "mc10ext.rom", OPEN_FLAG_READ, &rom);
		if( rom )
		{
			mame_fread(rom, memory_region(REGION_CPU1) + 0xc000, 0x2000);
			mame_fclose(rom);
		}
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_NOP);
    }
}

READ8_HANDLER ( mc10_bfff_r )
{
	/*   BIT 0 KEYBOARD ROW 1
	 *   BIT 1 KEYBOARD ROW 2
	 *   BIT 2 KEYBOARD ROW 3
	 *   BIT 3 KEYBOARD ROW 4
	 *   BIT 4 KEYBOARD ROW 5
	 *   BIT 5 KEYBOARD ROW 6
	 */

	int val = 0x40;

	if ((input_port_0_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x01;
	if ((input_port_1_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;
	if ((input_port_2_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x04;
	if ((input_port_3_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x08;
	if ((input_port_4_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x10;
	if ((input_port_5_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x20;

	return val;
}

READ8_HANDLER ( mc10_port1_r )
{
	return mc10_keyboard_strobe;
}

WRITE8_HANDLER ( mc10_port1_w )
{
	/*   BIT 0  KEYBOARD COLUMN 1 STROBE
	 *   BIT 1  KEYBOARD COLUMN 2 STROBE
	 *   BIT 2  KEYBOARD COLUMN 3 STROBE
	 *   BIT 3  KEYBOARD COLUMN 4 STROBE
	 *   BIT 4  KEYBOARD COLUMN 5 STROBE
	 *   BIT 5  KEYBOARD COLUMN 6 STROBE
	 *   BIT 6  KEYBOARD COLUMN 7 STROBE
	 *   BIT 7  KEYBOARD COLUMN 8 STROBE
	 */
	mc10_keyboard_strobe = data;
}

READ8_HANDLER ( mc10_port2_r )
{
	/*   BIT 1 KEYBOARD SHIFT/CONTROL KEYS INPUT
	 * ! BIT 2 PRINTER OTS INPUT
	 * ! BIT 3 RS232 INPUT DATA
	 *   BIT 4 CASSETTE TAPE INPUT
	 */

	mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);
	int val = 0xed;

	if ((input_port_6_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;

	if (cassette_input(img) >= 0)
		val |= 0x10;

	return val;
}

WRITE8_HANDLER ( mc10_port2_w )
{
	mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);

	/*   BIT 0 PRINTER OUTFUT & CASS OUTPUT
	 */

	cassette_output(img, (data & 0x01) ? +1.0 : -1.0);
}



/* --------------------------------------------------
 * Video hardware
 * -------------------------------------------------- */

static ATTR_CONST UINT8 mc10_get_attributes(UINT8 c)
{
	UINT8 result = 0x00;
	if (c & 0x40)			result |= M6847_INV;
	if (c & 0x80)			result |= M6847_AS;
	if (mc10_bfff & 0x04)	result |= M6847_GM2 | M6847_INTEXT;
	if (mc10_bfff & 0x08)	result |= M6847_GM1;
	if (mc10_bfff & 0x10)	result |= M6847_GM0;
	if (mc10_bfff & 0x20)	result |= M6847_AG;
	if (mc10_bfff & 0x40)	result |= M6847_CSS;
	return result;
}



static const UINT8 *mc10_get_video_ram(int scanline)
{
	offs_t offset = 0;

	switch(mc10_bfff & 0x3C)
	{
		case 0x00: case 0x04: case 0x08: case 0x0C:
		case 0x10: case 0x14: case 0x18: case 0x1C:
			offset = (scanline / 12) * 0x20;	/* text/semigraphic */
			break;

		case 0x20:	offset = (scanline / 3) * 0x10;	break;	/* CG1 */
		case 0x30:	offset = (scanline / 3) * 0x10;	break;	/* RG1 */
		case 0x28:	offset = (scanline / 3) * 0x20;	break;	/* CG2 */
		case 0x38:	offset = (scanline / 2) * 0x20;	break;	/* RG2 */
		case 0x24:	offset = (scanline / 2) * 0x20;	break;	/* CG3 */
		case 0x34:	offset = (scanline / 1) * 0x10;	break;	/* RG3 */
		case 0x2C:	offset = (scanline / 1) * 0x20;	break;	/* CG6 */
		case 0x3C:	offset = (scanline / 1) * 0x20;	break;	/* RG6 */
	}

	return mess_ram + offset;
}



VIDEO_START( mc10 )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_NTSC;
	cfg.get_attributes = mc10_get_attributes;
	cfg.get_video_ram = mc10_get_video_ram;
	m6847_init(&cfg);
	return 0;
}



WRITE8_HANDLER ( mc10_bfff_w )
{
	/*   BIT 2 GM2 6847 CONTROL & INT/EXT CONTROL
	 *   BIT 3 GM1 6847 CONTROL
	 *   BIT 4 GM0 6847 CONTROL
	 *   BIT 5 A/G 684? CONTROL
	 *   BIT 6 CSS 6847 CONTROL
	 *   BIT 7 SOUND OUTPUT BIT
	 */
	mc10_bfff = data;
	DAC_data_w(0, data & 0x80);
}



MACHINE_START( mc10 )
{
	mc10_bfff = 0x00;
	mc10_keyboard_strobe = 0x00;

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1,
		0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1,
		0, 0, MWA8_BANK1);
	memory_set_bankptr(1, mess_ram);

	state_save_register_global(mc10_bfff);
	state_save_register_global(mc10_keyboard_strobe);
}
