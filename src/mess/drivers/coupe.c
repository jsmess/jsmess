/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton


	Sam Coupe Memory Map - Based around the current spectrum.c (for obvious reasons!!)

	CPU:
		0000-7fff Banked rom/ram
		8000-ffff Banked rom/ram


Interrupts:

Changes:

 V0.2	- Added FDC support. - Based on 1771 document. Coupe had a 1772... (any difference?)
		  	floppy supports only read sector single mode at present will add write sector
			in next version.
		  Fixed up palette - had red & green wrong way round.


 KT 26-Aug-2000 - Changed to use wd179x code. This is the same as the 1772.
                - Coupe supports the basic disk image format, but can be changed in
                  the future to support others
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "includes/coupe.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "sound/saa1099.h"
#include "sound/speaker.h"

ADDRESS_MAP_START( coupe_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3FFF) AM_RAMBANK(1)
	AM_RANGE( 0x4000, 0x7FFF) AM_RAMBANK(2)
	AM_RANGE( 0x8000, 0xBFFF) AM_RAMBANK(3)
	AM_RANGE( 0xC000, 0xFFFF) AM_RAMBANK(4)
ADDRESS_MAP_END

static INTERRUPT_GEN( coupe_line_interrupt )
{
	mame_bitmap *bitmap = tmpbitmap;
	int interrupted=0;	/* This is used to allow me to clear the STAT flag (easiest way I can do it!) */

	HPEN = CURLINE;

	if (LINE_INT<192)
	{
		if (CURLINE == LINE_INT)
		{
			/* No other interrupts can occur - NOT CORRECT!!! */
            STAT=0x1E;
			cpunum_set_input_line(0, 0, HOLD_LINE);
			interrupted=1;
		}
	}

	/* scan line on screen so draw last scan line (may need to alter this slightly!!) */
    if (CURLINE && (CURLINE-1) < 192)
	{
		switch ((VMPR & 0x60)>>5)
		{
		case 0: /* mode 1 */
			drawMode1_line(bitmap,(CURLINE-1));
			break;
		case 1: /* mode 2 */
			drawMode2_line(bitmap,(CURLINE-1));
			break;
		case 2: /* mode 3 */
			drawMode3_line(bitmap,(CURLINE-1));
			break;
		case 3: /* mode 4 */
			drawMode4_line(bitmap,(CURLINE-1));
			break;
		}
	}

	CURLINE = (CURLINE + 1) % (192+10);

	if (CURLINE == 193)
	{
		if (interrupted)
			STAT&=~0x08;
		else
			STAT=0x17;

		cpunum_set_input_line(0, 0, HOLD_LINE);
		interrupted=1;
	}

	if (!interrupted)
		STAT=0x1F;
}

static unsigned char getSamKey1(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
		result &=readinputport(8) & 0x1F;
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0x1F;
		if (hi&0x40) result &= readinputport(6) & 0x1F;
		if (hi&0x20) result &= readinputport(5) & 0x1F;
		if (hi&0x10) result &= readinputport(4) & 0x1F;
		if (hi&0x08) result &= readinputport(3) & 0x1F;
		if (hi&0x04) result &= readinputport(2) & 0x1F;
		if (hi&0x02) result &= readinputport(1) & 0x1F;
		if (hi&0x01) result &= readinputport(0) & 0x1F;
	}

	return result;
}

static unsigned char getSamKey2(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
	{
		/* does not map to any keys? */
	}
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0xE0;
		if (hi&0x40) result &= readinputport(6) & 0xE0;
		if (hi&0x20) result &= readinputport(5) & 0xE0;
		if (hi&0x10) result &= readinputport(4) & 0xE0;
		if (hi&0x08) result &= readinputport(3) & 0xE0;
		if (hi&0x04) result &= readinputport(2) & 0xE0;
		if (hi&0x02) result &= readinputport(1) & 0xE0;
		if (hi&0x01) result &= readinputport(0) & 0xE0;
	}

	return result;
}


static READ8_HANDLER( coupe_port_r )
{
    if (offset==SSND_ADDR)  /* Sound address request */
		return SOUND_ADDR;

	if (offset==HPEN_PORT)
		return HPEN;

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:	/* This covers the total range of ports for 1 floppy controller */
    case DSK1_PORT+4:
		wd179x_set_side((offset >> 2) & 1);
		return wd179x_status_r(0);
	case DSK1_PORT+1:
    case DSK1_PORT+5:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_track_r(0);
	case DSK1_PORT+2:
    case DSK1_PORT+6:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_sector_r(0);
	case DSK1_PORT+3:
	case DSK1_PORT+7:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_data_r(0);
	case LPEN_PORT:
		return LPEN;
	case STAT_PORT:
		return ((getSamKey2((offset >> 8)&0xFF))&0xE0) | STAT;
	case LMPR_PORT:
		return LMPR;
	case HMPR_PORT:
		return HMPR;
	case VMPR_PORT:
		return VMPR;
	case KEYB_PORT:
		return (getSamKey1((offset >> 8)&0xFF)&0x1F) | 0xE0;
	case SSND_DATA:
		return SOUND_REG[SOUND_ADDR];
	default:
		logerror("Read Unsupported Port: %04x\n", offset);
		break;
	}

	return 0x0ff;
}


static WRITE8_HANDLER( coupe_port_w )
{
	if (offset==SSND_ADDR)						// Set sound address
	{
		SOUND_ADDR=data&0x1F;					// 32 registers max
		saa1099_control_port_0_w(0, SOUND_ADDR);
        return;
	}

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:							// This covers the total range of ports for 1 floppy controller
    case DSK1_PORT+4:
		wd179x_set_side((offset >> 2) & 1);
        wd179x_command_w(0, data);
		break;
    case DSK1_PORT+1:
    case DSK1_PORT+5:
		/* Track byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_track_w(0, data);
		break;
    case DSK1_PORT+2:
    case DSK1_PORT+6:
		/* Sector byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_sector_w(0, data);
        break;
    case DSK1_PORT+3:
	case DSK1_PORT+7:
		/* Data byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_data_w(0, data);
		break;
	case CLUT_PORT:
		CLUT[(offset >> 8)&0x0F]=data&0x7F;		// set CLUT data
		break;
	case LINE_PORT:
		LINE_INT=data;						// Line to generate interrupt on
		break;
    case LMPR_PORT:
		LMPR=data;
		coupe_update_memory();
		break;
    case HMPR_PORT:
		HMPR=data;
		coupe_update_memory();
		break;
    case VMPR_PORT:
		VMPR=data;
		coupe_update_memory();
		break;
    case BORD_PORT:
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
		break;
    case SSND_DATA:
		saa1099_write_port_0_w(0, data);
		SOUND_REG[SOUND_ADDR] = data;
		break;
    default:
		logerror("Write Unsupported Port: %04x,%02x\n", offset,data);
		break;
	}
}

ADDRESS_MAP_START( coupe_io , ADDRESS_SPACE_IO, 8)	
	AM_RANGE( 0x0000, 0x0ffff) AM_READWRITE( coupe_port_r, coupe_port_w )
ADDRESS_MAP_END

static gfx_decode coupe_gfxdecodeinfo[] = {
	{-1}
};	 /* end of array */

INPUT_PORTS_START( coupe )
	PORT_START // FE  0
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)

	PORT_START // FD  1
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)

	PORT_START // FB  2
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)

	PORT_START // F7  3
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)

	PORT_START // EF  4
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)

	PORT_START // DF  5
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\"") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0") PORT_CODE(KEYCODE_F10)

	PORT_START // BF  6
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT") PORT_CODE(KEYCODE_RALT)

	PORT_START // 7F  7
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV") PORT_CODE(KEYCODE_SLASH)

	PORT_START // FF  8
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)

INPUT_PORTS_END

/* Initialise the palette */
static PALETTE_INIT( coupe )
{
	unsigned char red,green,blue;
	int a;
	unsigned short coupe_colortable[128];		// 1-1 relationship to palette!

	for (a=0;a<128;a++)
	{
		/* decode colours for palette as follows :
		 * bit number		7		6		5		4		3		2		1		0
		 *						|		|		|		|		|		|		|
		 *				 nothing   G+4	   R+4	   B+4	  ALL+1    G+2	   R+2	   B+2
		 *
		 * these values scaled up to 0-255 range would give modifiers of :	+4 = +(4*36), +2 = +(2*36), +1 = *(1*36)
		 * not quite max of 255 but close enough for me!
		 */
		red=green=blue=0;
		if (a&0x01)
			blue+=2*36;
		if (a&0x02)
			red+=2*36;
		if (a&0x04)
			green+=2*36;
		if (a&0x08)
		{
			red+=1*36;
			green+=1*36;
			blue+=1*36;
		}
		if (a&0x10)
			blue+=4*36;
		if (a&0x20)
			red+=4*36;
		if (a&0x40)
			green+=4*36;

		palette_set_color(machine, a, red, green, blue);
		coupe_colortable[a]=a;
	}
	memcpy(colortable,coupe_colortable,sizeof(coupe_colortable));
}



static MACHINE_DRIVER_START( coupe )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)        /* 6 Mhz */
	MDRV_CPU_PROGRAM_MAP(coupe_mem, 0)
	MDRV_CPU_IO_MAP(coupe_io, 0)
	MDRV_CPU_VBLANK_INT(coupe_line_interrupt, 192 + 10)	/* 192 scanlines + 10 lines of vblank (approx).. */
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( coupe )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 24*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 24*8-1)
	MDRV_GFXDECODE( coupe_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(128)
	MDRV_COLORTABLE_LENGTH(128)
	MDRV_PALETTE_INIT(coupe)

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SAA1099, 8000000 /* guess */)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(coupe)
	ROM_REGION(0x8000,REGION_CPU1,0)
	ROM_LOAD("sam_rom0.rom", 0x0000, 0x4000, CRC(9954CF1A) SHA1(d92e3fd2a65dbf439d7050d9cea732a63640d567))
	ROM_LOAD("sam_rom1.rom", 0x4000, 0x4000, CRC(F031AED4) SHA1(a7f06facc6f0a3713215f4befc307914836706a4))
ROM_END

static void coupe_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_coupe_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(coupe)
	CONFIG_RAM_DEFAULT(256 * 1024)
	CONFIG_RAM(512 * 1024)

	CONFIG_DEVICE(coupe_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE         INPUT     INIT  CONFIG  COMPANY                 		  FULLNAME */
COMP( 1989, coupe,	  0,		0,		coupe,			coupe,	  0,	coupe,	"Miles Gordon Technology plc",    "Sam Coupe" , 0)
