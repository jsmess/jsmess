/*

    Hitachi HD66421 LCD Controller/Driver

    (c) 2001-2007 Tim Schuerewegen

*/

#include "emu.h"
#include "hd66421.h"

#define LOG_LEVEL  1
#define _logerror(level,x)  do { if (LOG_LEVEL > level) logerror x; } while (0)

//#define BRIGHTNESS_DOES_NOT_WORK

#define HD66421_RAM_SIZE  (HD66421_WIDTH * HD66421_HEIGHT / 4) // 2-bits per pixel

// R0 - control register 1
#define LCD_R0_RMW      0x80 // read-modify-write mode
#define LCD_R0_DISP     0x40 // display on/off
#define LCD_R0_STBY     0x20 // standby (internal operation and power circuit halt)
#define LCD_R0_PWR      0x10
#define LCD_R0_AMP      0x08
#define LCD_R0_REV      0x04 // reverse
#define LCD_R0_HOLT     0x02
#define LCD_R0_ADC      0x01

// R1 - control register 2
#define LCD_R1_BIS1     0x80 // bias ratio (bit 1)
#define LCD_R1_BIS0     0x40 // bias ratio (bit 0)
#define LCD_R1_WLS      0x20
#define LCD_R1_GRAY     0x10 // grayscale palette 4/32
#define LCD_R1_DTY1     0x08 // display duty cycle (bit 1)
#define LCD_R1_DTY0     0x04 // display duty cycle (bit 0)
#define LCD_R1_INC      0x02
#define LCD_R1_BLK      0x01 // blink function

// register 0 to 16
#define LCD_REG_CONTROL_1   0x00 // control register 1
#define LCD_REG_CONTROL_2   0x01 // control register 2
#define LCD_REG_ADDR_X      0x02 // x address register
#define LCD_REG_ADDR_Y      0x03 // y address register
#define LCD_REG_RAM         0x04 // display ram access register
#define LCD_REG_START_Y     0x05 // display start line register
#define LCD_REG_BLINK_START 0x06 // blink start line register
#define LCD_REG_BLINK_END   0x07 // blink end line register
#define LCD_REG_BLINK_1     0x08 // blink register 1
#define LCD_REG_BLINK_2     0x09 // blink register 2
#define LCD_REG_BLINK_3     0x0A // blink register 3
#define LCD_REG_PARTIAL     0x0B // partial display block register
#define LCD_REG_COLOR_1     0x0C // gray scale palette 1 (0,0)
#define LCD_REG_COLOR_2     0x0D // gray scale palette 2 (0,1)
#define LCD_REG_COLOR_3     0x0E // gray scale palette 3 (1,0)
#define LCD_REG_COLOR_4     0x0F // gray scale palette 4 (1,1)
#define LCD_REG_CONTRAST    0x10 // contrast control register
#define LCD_REG_PLANE       0x11 // plane selection register

typedef struct
{
	UINT8 idx, dat[32]; // index and data registers
	UINT8 *ram;         // display ram
	UINT32 pos;         // current position in ram
} HD66421;

static HD66421 lcd;

UINT8 hd66421_reg_idx_r(void)
{
	_logerror( 2, ("hd66421_reg_idx_r\n"));
	return lcd.idx;
}

void hd66421_reg_idx_w(UINT8 data)
{
	_logerror( 2, ("hd66421_reg_idx_w (%02X)\n", data));
	lcd.idx = data;
}

UINT8 hd66421_reg_dat_r(void)
{
	_logerror( 2, ("hd66421_reg_dat_r\n"));
	return lcd.dat[lcd.idx];
}

void hd66421_reg_dat_w(UINT8 data)
{
	_logerror( 2, ("hd66421_reg_dat_w (%02X)\n", data));
	lcd.dat[lcd.idx] = data;
	switch (lcd.idx)
	{
		case LCD_REG_ADDR_Y : lcd.pos = data * HD66421_WIDTH / 4; break;
		case LCD_REG_RAM    : lcd.ram[lcd.pos++] = data; break;
	}
}

INLINE void hd66421_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16( bitmap, y, x) = (UINT16)color;
}

PALETTE_INIT( hd66421 )
{
	int i;
	_logerror( 0, ("palette_init_hd66421\n"));
	// init palette
	for (i=0;i<4;i++)
	{
		palette_set_color( machine, i, RGB_WHITE);
		#ifndef BRIGHTNESS_DOES_NOT_WORK
		palette_set_pen_contrast( machine, i, 1.0 * i / (4 - 1));
		#endif
	}
}

static void hd66421_state_save(running_machine &machine)
{
	const char *name = "hd66421";
	state_save_register_item(machine, name, NULL, 0, lcd.idx);
	state_save_register_item_array(machine, name, NULL, 0, lcd.dat);
	state_save_register_item(machine, name, NULL, 0, lcd.pos);
	state_save_register_item_pointer(machine, name, NULL, 0, lcd.ram, HD66421_RAM_SIZE);
}

VIDEO_START( hd66421 )
{
	_logerror( 0, ("video_start_hd66421\n"));
	memset( &lcd, 0, sizeof( lcd));
	lcd.ram = auto_alloc_array(machine, UINT8, HD66421_RAM_SIZE);
	hd66421_state_save(machine);
}

SCREEN_UPDATE( hd66421 )
{
	pen_t pen[4];
	int i;
	_logerror( 1, ("video_update_hd66421\n"));
	// update palette
	for (i=0;i<4;i++)
	{
		double bright;
		int temp;
		temp = 31 - (lcd.dat[LCD_REG_COLOR_1+i] - lcd.dat[LCD_REG_CONTRAST] + 0x03);
		if (temp <  0) temp =  0;
		if (temp > 31) temp = 31;
		bright = 1.0 * temp / 31;
		pen[i] = i;
		#ifdef BRIGHTNESS_DOES_NOT_WORK
		palette_set_color(screen->machine(), pen[i], 255 * bright, 255 * bright, 255 * bright);
		#else
		palette_set_pen_contrast(screen->machine(), pen[i], bright);
		#endif
	}
	// draw bitmap (bottom to top)
	if (lcd.dat[0] & LCD_R0_DISP)
	{
		int x, y;
		x = 0;
		y = HD66421_HEIGHT - 1;
		for (i=0;i<HD66421_RAM_SIZE;i++)
		{
			hd66421_plot_pixel( bitmap, x++, y, pen[(lcd.ram[i] >> 6) & 3]);
			hd66421_plot_pixel( bitmap, x++, y, pen[(lcd.ram[i] >> 4) & 3]);
			hd66421_plot_pixel( bitmap, x++, y, pen[(lcd.ram[i] >> 2) & 3]);
			hd66421_plot_pixel( bitmap, x++, y, pen[(lcd.ram[i] >> 0) & 3]);
			if (x >= HD66421_WIDTH)
			{
				x = 0;
				y = y - 1;
			}
		}
	}
	else
	{
		rectangle rect;
		rect.min_x = 0;
		rect.max_x = HD66421_WIDTH - 1;
		rect.min_y = 0;
		rect.max_y = HD66421_HEIGHT - 1;
		bitmap_fill( bitmap, &rect, get_white_pen(screen->machine()));
	}
	// flags
	return 0;
}
