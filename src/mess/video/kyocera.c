#include "driver.h"
#include "includes/kyocera.h"

static bitmap_t *tmpbitmap;
static int bank;
static UINT8 lcd[11][256];
static UINT8 lcd_addr[10];

void kyo85_set_lcd_bank(UINT16 data)
{
	switch (data) 
	{
		case 0x000:	bank = 0; break;
		case 0x001:	bank = 1; break;
		case 0x002:	bank = 2; break;
		case 0x004:	bank = 3; break;
		case 0x008:	bank = 4; break;
		case 0x010:	bank = 5; break;
		case 0x020:	bank = 6; break;
		case 0x040:	bank = 7; break;
		case 0x080:	bank = 8; break;
		case 0x100:	bank = 9; break;
		case 0x200:	bank = 10; break;
		default: /*logerror("pc8201: bank update with unk bank = %d\n", bank);*/ break;
	}
}

static void increase(void) {
	UINT8 addr = lcd_addr[bank];
	int col = addr & 0x3f;
	int row = addr >> 6;

	col++;

	if (bank == 5 || bank == 10)
	{
		if (col == 40)
		{
			col = 0;
			row++;
		}
	}
	else
	{
		if (col == 50)
		{
			col = 0;
			row++;
		}
	}

	lcd_addr[bank] = (row << 6) | col;
}

READ8_HANDLER( kyo85_lcd_status_r )
{
	return 0; // always ready
}

READ8_HANDLER( kyo85_lcd_data_r )
{
	UINT8 addr = lcd_addr[bank];

	//logerror("LCD driver %u column %u row %u data read %02x\n", bank, addr & 0x3f, addr >> 6, lcd[bank][addr]);

	increase();

	return lcd[bank][addr];
}

WRITE8_HANDLER( kyo85_lcd_command_w )
{
	/*

		bit		description

		0		column bit 0
		1		column bit 1
		2		column bit 2
		3		column bit 3
		4		column bit 4
		5		column bit 5
		6		row bit 0
		7		row bit 1

	*/

	lcd_addr[bank] = data;

	//logerror("LCD driver %u column %u row %u\n", bank, data & 0x3f, data >> 6);
}

WRITE8_HANDLER( kyo85_lcd_data_w )
{
	UINT8 addr = lcd_addr[bank];

	int col = addr & 0x3f;
	int row = addr >> 6;
	int sx, sy, y;
	
	if (bank > 5)
	{
		sx = (bank - 6) * 50;
		sy = 32;
	}
	else
	{
		sx = (bank - 1) * 50;
		sy = 0;
	}

	sx += col;
	sy += row * 8;

	lcd[bank][addr] = data;
	
	for (y = 0; y < 8; y++)
	{
		int color = BIT(data, y);
		*BITMAP_ADDR16(tmpbitmap, sy + y, sx) = color;
	}

	//logerror("LCD driver %u column %u row %u data write %02x\n", bank, addr & 0x3f, addr >> 6, data);

	increase();
}

PALETTE_INIT( kyo85 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

VIDEO_START( kyo85 )
{
	/* allocate the temporary bitmap */
	tmpbitmap = auto_bitmap_alloc(240, 64, BITMAP_FORMAT_INDEXED16);
}

VIDEO_UPDATE( kyo85 )
{
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect);

	return 0;
}
