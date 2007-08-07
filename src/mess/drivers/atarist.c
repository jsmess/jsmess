#include "driver.h"
#include "inputx.h"
#include "video/generic.h"
#include "video/atarist.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/printer.h"
#include "includes/centroni.h"
#include "includes/serial.h"
#include "machine/6850acia.h"
#include "machine/68901mfp.h"
#include "machine/wd17xx.h"
#include "sound/ay8910.h"

/*

	TODO:

	- US keyboard layout
	- HD6310 cpu core
	- connect keyboard ports to HD6310 (needs port3/4 added to the cpu core)
	- connect mouse to HD6310
	- connect HD6310 to ACIA #0
	- floppy image device_load
	- MFP interrupts
	- MMU
	- accurate screen timing

*/

#define Y1		2457600.0
#define Y2		32084988.0
#define Y2_NTSC	32042400.0

/* WD1772 FDC */

static struct FDC
{
	int dma_status;
	int reg;
	int dma_sector_count;
	int dma_select;
	int dma_direction;
	UINT32 dma_base;
	int dma_bytes;
	int dma_int;
	int active;
} fdc;

static void atarist_dma_transfer(void)
{
	UINT8 *RAM = memory_region(REGION_CPU1) + fdc.dma_base;

	if (fdc.dma_direction)
	{
		wd17xx_data_w(0, RAM[0]);
	}
	else
	{
		RAM[0] = wd17xx_data_r(0);
	}

	fdc.dma_base++;
	fdc.dma_bytes--;

	if (fdc.dma_bytes == 0)
	{
		fdc.dma_int = ASSERT_LINE;
	}
	else
	{
		fdc.dma_int = CLEAR_LINE;
	}
}

static void atarist_fdc_callback(wd17xx_state_t event, void *param)
{
	switch (event)
	{
	case WD17XX_IRQ_CLR:
	case WD17XX_DRQ_CLR:
		break;
	case WD17XX_IRQ_SET:
		logerror("WD1792:  Warning: IRQ set!\n");
		break;
	case WD17XX_DRQ_SET:
		if (fdc.dma_select == 0)
		{
			atarist_dma_transfer();
		}
		break;
	}
}

static READ16_HANDLER( atarist_fdc_r )
{
	switch (offset)
	{
	case 0: // reserved
	case 2: // reserved
		break;

	case 4: // data register
		fdc.active = 0; //todo?

		switch (fdc.reg & 0x0f)
		{
		// A0/A1 pins on wd179x controller
		case 0: 
			fdc.dma_status = 1; 
			return wd17xx_status_r(0);
		case 1: 
			fdc.dma_status = 1; 
			return wd17xx_track_r(0);
		case 2: 
			fdc.dma_status = 1;
			return wd17xx_sector_r(0);
		case 3: 
			fdc.dma_status = 1; 
			return wd17xx_data_r(0);

		// HDC register select - Unimplemented
		case 4:
		case 5:
		case 6:
		case 7:
			fdc.dma_status = 0; // Force DMA error on hard-disk accesses
			break;

		// DMA sector count register
		default:
			fdc.dma_status = 1;
			return fdc.dma_sector_count;
		}
		break;

	case 6: // Status register
		return 0xfe | fdc.dma_status;
	}

	return 0xff;
}

static WRITE16_HANDLER( atarist_fdc_w )
{
	switch (offset)
	{
	case 0: // Unused
	case 2:
		break;
	case 4: // Data register
		switch (fdc.reg & 0x0f)
		{
		// A0/A1 pins on wd179x controller
		case 0: 
			wd17xx_command_w(0, data & 0xff);
			break;
		case 1: 
			wd17xx_track_w(0, data & 0xff); 
			break;
		case 2: 
			wd17xx_sector_w(0, data & 0xff);
			break;
		case 3: 
			wd17xx_data_w(0, data & 0xff); 
			break;
		// HDC register select - Unimplemented
		case 4:
		case 5:
		case 6:
		case 7:
			break;
		// DMA sector count register
		default:
			fdc.dma_sector_count = data & 0xff;
			break;
		}
		break;
	case 6: // Select/Status register
		fdc.reg = (data >> 1) & 0x0f;
		fdc.dma_select = data & 0x40;
		fdc.dma_direction = data & 0x100;
		break;
	case 8:
		fdc.dma_base = (fdc.dma_base & 0x00ffff) | ((data & 0xff) << 16);
		fdc.dma_bytes = 512;
		break;
	case 10:
		fdc.dma_base = (fdc.dma_base & 0xff00ff) | ((data & 0xff) << 8);
		fdc.dma_bytes = 512;
		break;
	case 12:
		fdc.dma_base = (fdc.dma_base & 0xffff00) | (data & 0xff);
		fdc.dma_bytes = 512;
		break;
	}
}

/* SHIFTER */


/* MMU */

static int mmu;

static READ16_HANDLER( atarist_mmu_r )
{
	return mmu;
}

static WRITE16_HANDLER( atarist_mmu_w )
{
	mmu = data & 0xff;
}

/* Memory Maps */

static UINT8 keylatch;
static int joylatch;

static WRITE8_HANDLER( hd6301_port_1_w )
{
	keylatch = data;
}

static READ8_HANDLER( hd6301_port2_r )
{
	/*
		
		bit		description
		
		0		JOY 1-5
		1		JOY 0-6
		2		JOY 1-6
		3		SD FROM CPU
		4		SD TO CPU

	*/

	UINT8 data = 0;
		
//	data |= acia_ikbd_tx << 3;
	
	return data;
}

static WRITE8_HANDLER( hd6301_port2_w )
{
	/*
		
		bit		description
		
		0		JOY 1-5
		1		JOY 0-6
		2		JOY 1-6
		3		SD FROM CPU
		4		SD TO CPU

	*/

	joylatch = data & 0x01;
//	acia_ikbd_rx = (data & 0x10) >> 4;
}

static WRITE8_HANDLER( hd6301_port_3_w )
{
	// 0x01 CAPS LOCK led
}

static READ8_HANDLER( hd6301_port_3_r )
{
	/*
		
		bit		description
		
		0		CAPS LOCK LED
		1		Keyboard row input
		2		Keyboard row input
		3		Keyboard row input
		4		Keyboard row input
		5		Keyboard row input
		6		Keyboard row input
		7		Keyboard row input

	*/

	return 0xff;
}

static READ8_HANDLER( hd6301_port_4_r )
{
	/*
		
		bit		description
		
		0		JOY 0-1 or keyboard row input
		1		JOY 0-2 or keyboard row input
		2		JOY 0-3 or keyboard row input
		3		JOY 0-4 or keyboard row input
		4		JOY 1-1 or keyboard row input
		5		JOY 1-2 or keyboard row input
		6		JOY 1-3 or keyboard row input
		7		JOY 1-4 or keyboard row input

	*/

	return 0xff;
}

static ADDRESS_MAP_START(keyboard_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(keyboard_io_map, ADDRESS_SPACE_IO, 8)
/*	AM_RANGE(HD6301_PORT1, HD6301_PORT1) AM_WRITE(keyboard_latch_w)
	AM_RANGE(HD6301_PORT2, HD6301_PORT2) AM_READWRITE(hd6301_port2_r, hd6301_port2_w)
	AM_RANGE(HD6301_PORT3, HD6301_PORT3) AM_READ(keyboard_0_r)
	AM_RANGE(HD6301_PORT4, HD6301_PORT4) AM_READ(keyboard_1_r)*/
ADDRESS_MAP_END

static ADDRESS_MAP_START(st_map, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x0fffff) AM_RAMBANK(1)
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(2)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff82ff) AM_READWRITE(atarist_shifter_r, atarist_shifter_w)
	AM_RANGE(0xff8600, 0xff860f) AM_READWRITE(atarist_fdc_r, atarist_fdc_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE(AY8910_read_port_0_msb_r, AY8910_control_port_0_msb_w)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE(AY8910_write_port_0_msb_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_READWRITE(mfp68901_0_register16_r, mfp68901_0_register_msb_w)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE(acia6850_0_stat_16_r, acia6850_0_ctrl_msb_w)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE(acia6850_0_data_16_r, acia6850_0_data_msb_w)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE(acia6850_1_stat_16_r, acia6850_1_ctrl_msb_w)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE(acia6850_1_data_16_r, acia6850_1_data_msb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(megast_map, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x000000, 0x3fffff) AM_RAMBANK(1)
	AM_RANGE(0xfa0000, 0xfbffff) AM_ROMBANK(2)
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8007) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff82ff) AM_READWRITE(atarist_shifter_r, atarist_shifter_w)
	AM_RANGE(0xff8600, 0xff860f) AM_READWRITE(atarist_fdc_r, atarist_fdc_w)
	AM_RANGE(0xff8800, 0xff8801) AM_READWRITE(AY8910_read_port_0_msb_r, AY8910_control_port_0_msb_w)
	AM_RANGE(0xff8802, 0xff8803) AM_WRITE(AY8910_write_port_0_msb_w)
//	AM_RANGE(0xff8a00, 0xff8a3f) AM_READWRITE(atarist_blitter_r, atarist_blitter_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_READWRITE(mfp68901_0_register16_r, mfp68901_0_register_msb_w)
	AM_RANGE(0xfffc00, 0xfffc01) AM_READWRITE(acia6850_0_stat_16_r, acia6850_0_ctrl_msb_w)
	AM_RANGE(0xfffc02, 0xfffc03) AM_READWRITE(acia6850_0_data_16_r, acia6850_0_data_msb_w)
	AM_RANGE(0xfffc04, 0xfffc05) AM_READWRITE(acia6850_1_stat_16_r, acia6850_1_ctrl_msb_w)
	AM_RANGE(0xfffc06, 0xfffc07) AM_READWRITE(acia6850_1_data_16_r, acia6850_1_data_msb_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( keyboard )
	PORT_START_TAG("P31")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0xef, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P32")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0xde, IP_ACTIVE_LOW, IPT_UNUSED ) 

	PORT_START_TAG("P33")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(DEF_STR( Alternate )) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0xbe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P34")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("P35")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_START_TAG("P36")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START_TAG("P37")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6) PORT_NAME("F6")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START_TAG("P40")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7) PORT_NAME("F7")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START_TAG("P41")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8) PORT_NAME("F8")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START_TAG("P42")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F9) PORT_NAME("F9")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START_TAG("P43")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F10) PORT_NAME("F10")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('´') PORT_CHAR('`')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START_TAG("P44")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Help") PORT_CODE(KEYCODE_F11)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Insert") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START_TAG("P45")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Undo") PORT_CODE(KEYCODE_F12)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START_TAG("P46")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad (")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad )")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START_TAG("P47")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad /") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad Enter") PORT_CODE(KEYCODE_ENTER_PAD)
INPUT_PORTS_END

INPUT_PORTS_START( atarist )
	PORT_START_TAG("config")
	PORT_CONFNAME( 0x20, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x20, DEF_STR(Joystick) )

	PORT_START_TAG("MOUSE_X")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)	

	PORT_START_TAG("MOUSE_Y")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)	

	PORT_START_TAG("MOUSE_BUTTONS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	
	PORT_INCLUDE( keyboard )
INPUT_PORTS_END

INPUT_PORTS_START( joystick_ste )
	PORT_START_TAG("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("JOY1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(4) PORT_8WAY

	PORT_START_TAG("PADDLE0X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START_TAG("PADDLE0Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START_TAG("PADDLE1X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START_TAG("PADDLE1Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START_TAG("GUNX") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUNY") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

/* Sound Interface */

static WRITE8_HANDLER( ym2149_port_a_w )
{
	wd17xx_set_side(data & 0x01);

	if (data & 0x02)
	{
		wd17xx_set_drive(0);
	}
	else if (data & 0x04)
	{
		wd17xx_set_drive(1);
	}

	// 0x08 = RTS
	// 0x10 = DTR

	centronics_write_handshake(1, (data & 0x20) ? 0 : CENTRONICS_STROBE, CENTRONICS_STROBE);

	// 0x40 = General Purpose Output
	// 0x80 = Reserved
}

static WRITE8_HANDLER( ym2149_port_b_w )
{
	centronics_write_data(0, data);
}

static struct AY8910interface ym2149_interface =
{
	0,
	0,
	ym2149_port_a_w,
	ym2149_port_b_w
};

/* Machine Drivers */

static int acia_int;
static UINT8 acia_ikbd_rx, acia_ikbd_tx;
static UINT8 acia_midi_rx, acia_midi_tx;

static void acia_interrupt(int state)
{
	acia_int = state;
}

static struct acia6850_interface acia_ikbd_intf =
{
	500000,
	500000,
	&acia_ikbd_rx,
	&acia_ikbd_tx,
	acia_interrupt
};

static struct acia6850_interface acia_midi_intf =
{
	500000,
	500000,
	&acia_midi_rx,
	&acia_midi_tx,
	acia_interrupt
};

static READ8_HANDLER( mfp_gpio_r )
{
	/*

		bit		description
		
		0		Centronics BUSY
		1		RS232 DCD
		2		RS232 CTS
		3		Blitter done
		4		Keyboard/MIDI
		5		FDC
		6		RS232 RI
		7		Monochrome monitor detect

	*/

	UINT8 data = 0;
	int centronics_handshake = centronics_read_handshake(1);

	if ((centronics_handshake & CENTRONICS_NOT_BUSY) == 0)
	{
		data |= 0x01;
	}

	data |= acia_int << 4;
	data |= fdc.active << 5;
	data |= 0x80;

	return data;
}

static void mfp_interrupt(int which, int state, int vector)
{
	cpunum_set_input_line_vector(0, MC68000_IRQ_6, vector);
	cpunum_set_input_line(0, MC68000_IRQ_6, state);
}

static struct mfp68901_interface mfp_intf =
{
	Y2/32,
	Y1,
	NULL,
	NULL,
	NULL,
	NULL,
	mfp_interrupt,
	mfp_gpio_r,
	NULL
};

static MACHINE_START( atarist )
{
	memory_install_read16_handler (0, ADDRESS_SPACE_PROGRAM, 0x000000, mess_ram_size - 1, 0, 0, MRA16_BANK1);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, mess_ram_size - 1, 0, 0, MWA16_BANK1);

	if (mess_ram_size < 0x100000)
	{
		memory_install_read16_handler (0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x0fffff, 0, 0, MRA16_UNMAP);
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x0fffff, 0, 0, MWA16_UNMAP);
	}

	wd17xx_init(WD_TYPE_1772, atarist_fdc_callback, NULL);

	acia6850_config(0, &acia_ikbd_intf);
	acia6850_config(1, &acia_midi_intf);

	mfp68901_config(0, &mfp_intf);
}

static MACHINE_RESET( atarist )
{
	UINT8 *ROM = memory_region(REGION_CPU1) + 0xfc0000;

	memcpy(&mess_ram, ROM, 8);
}

static MACHINE_DRIVER_START( atarist )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M68000, Y2/4)
	MDRV_CPU_PROGRAM_MAP(st_map, 0)

	MDRV_CPU_ADD(HD63701, 1000000) // HD6301
	MDRV_CPU_PROGRAM_MAP(keyboard_map, 0)
	MDRV_CPU_IO_MAP(keyboard_io_map, 0)

	MDRV_MACHINE_START(atarist)
	MDRV_MACHINE_RESET(atarist)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( atarist )

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_RAW_PARAMS(Y2/4, 516, 0, 512, 313, 0, 312)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2149, Y2/16)
	MDRV_SOUND_CONFIG(ym2149_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)	
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megast )
	MDRV_IMPORT_FROM(atarist)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(megast_map, 0)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( atarist )
	ROM_REGION( 0x1000000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100.img", 0xfc0000, 0x030000, BAD_DUMP CRC(d331af30) SHA1(7bcc2311d122f451bd03c9763ade5a119b2f90da), ROM_BIOS(1) ) // this is a US rom
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.img", 0xfc0000, 0x030000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.img", 0xfc0000, 0x030000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(3) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast )
	ROM_REGION( 0x1000000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.img", 0xfc0000, 0x030000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.img", 0xfc0000, 0x030000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(2) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( atariste )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106.img", 0xe00000, 0x030000, BAD_DUMP CRC(de62800c) SHA1(7ade7f61dd99cb4e8e71513e74205349a6719cbb), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162.img", 0xe00000, 0x040000, BAD_DUMP CRC(d1c6f2fa) SHA1(70db24a7c252392755849f78940a41bfaebace71), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206.img", 0xe00000, 0x040000, BAD_DUMP CRC(08538e39) SHA1(2400ea95f547d6ea754a99d05d8530c03f8b28e3), ROM_BIOS(3) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos202", "TOS 2.02 (Mega STE TOS)" )
	ROMX_LOAD( "tos202.img", 0xe00000, 0x030000, BAD_DUMP CRC() SHA1(), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205.img", 0xe00000, 0x030000, BAD_DUMP CRC() SHA1(), ROM_BIOS(2) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( tt030 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos301", "TOS 3.01 (TT TOS)" )
	ROMX_LOAD( "tos301.img", 0xe00000, 0x080000, BAD_DUMP CRC() SHA1(), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos305", "TOS 3.05 (TT TOS)" )
	ROMX_LOAD( "tos305.img", 0xe00000, 0x080000, BAD_DUMP CRC() SHA1(), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.img", 0xe00000, 0x080000, BAD_DUMP CRC() SHA1(), ROM_BIOS(3) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( falcon )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_16BIT )
	ROM_SYSTEM_BIOS( 0, "tos400", "TOS 4.00" )
	ROMX_LOAD( "tos400.img", 0xe00000, 0x080000, BAD_DUMP CRC() SHA1(), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos401", "TOS 4.01" )
	ROMX_LOAD( "tos401.img", 0xe00000, 0x080000, BAD_DUMP CRC() SHA1(), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 2, "tos402", "TOS 4.02" )
	ROMX_LOAD( "tos402.img", 0xe00000, 0x080000, BAD_DUMP CRC(63f82f23) SHA1(75de588f6bbc630fa9c814f738195da23b972cc6), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 3, "tos404", "TOS 4.04" )
	ROMX_LOAD( "tos404.img", 0xe00000, 0x080000, BAD_DUMP CRC(028b561d) SHA1(27dcdb31b0951af99023b2fb8c370d8447ba6ebc), ROM_BIOS(1) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

/* System Configuration */

static DEVICE_LOAD( atarist_floppy )
{
	int tracks = 80 , heads = 2, sectors = 16;

	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		/* drive, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
		basicdsk_set_geometry(image, tracks, heads, sectors, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void atarist_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_atarist_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "st"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void atarist_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_LOAD( atarist_serial )
{
	/* filename specified */
	if (serial_device_load(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void atarist_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = serial_device_init; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_atarist_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = serial_device_unload; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static DEVICE_LOAD( atarist_cart )
{
	UINT8 *ptr = ((UINT8 *)memory_region(REGION_CPU1)) + 0xfa0000;
	int	filesize = image_length(image);

	if (filesize <= 128 * 1024)
	{
		if (image_fread(image, ptr, filesize) == filesize)
		{
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void atarist_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_atarist_cart;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "stc");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START( atarist )
	CONFIG_RAM_DEFAULT(1024 * 1024) // 1040ST
	CONFIG_RAM		  ( 512 * 1024) //  520ST
	CONFIG_RAM		  ( 256 * 1024) //  260ST
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(atarist_printer_getinfo)
	CONFIG_DEVICE(atarist_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( megast )
	CONFIG_RAM_DEFAULT(4096 * 1024) // Mega ST 4
	CONFIG_RAM		  (2048 * 1024) // Mega ST 2
	CONFIG_RAM		  (1024 * 1024) // Mega ST 1
	CONFIG_DEVICE(atarist_floppy_getinfo)
	CONFIG_DEVICE(atarist_printer_getinfo)
	CONFIG_DEVICE(atarist_serial_getinfo)
	CONFIG_DEVICE(atarist_cartslot_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*     YEAR  NAME    PARENT    COMPAT	MACHINE   INPUT     INIT	CONFIG   COMPANY    FULLNAME */
COMP( 1985, atarist,  0,        0,		atarist,  atarist,  0,     atarist,  "Atari", "ST", GAME_NOT_WORKING )
COMP( 1987, megast,   atarist,  0,		megast,   atarist,  0,     megast,   "Atari", "Mega ST", GAME_NOT_WORKING )
/*
COMP( 1989, atariste, 0,		0,		atariste, atarist,  0,     atariste, "Atari", "STE", GAME_NOT_WORKING )
COMP( 1991, megaste,  atariste, 0,		megaste,  atarist,  0,     megaste,  "Atari", "Mega STE", GAME_NOT_WORKING )
COMP( 1989, stacy,    0,        0,		stacy,    stacy,    0,     stacy,	 "Atari", "STacy", GAME_NOT_WORKING )
COMP( 1992, stbook,   0,        0,		stbook,   stbook,   0,     stbook,	 "Atari", "ST Book", GAME_NOT_WORKING )
COMP( 1990, tt030,    0,        0,		tt030,    tt030,    0,     tt030,	 "Atari", "TT030", GAME_NOT_WORKING )
COMP( 1992, falcon,   0,        0,		falcon,   falcon,   0,     falcon,	 "Atari", "Falcon030", GAME_NOT_WORKING )
*/
