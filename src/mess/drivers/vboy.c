/***************************************************************************
   
	Nintendo Virtual Boy

    12/05/2009 Skeleton driver.

    Great info at http://www.goliathindustries.com/vb/ and http://www.vr32.de/modules/dokuwiki/doku.php?   

****************************************************************************/

#include "driver.h"
#include "cpu/v810/v810.h"
#include "devices/cartslot.h"
#include "vboy.lh"

UINT16 *vboy_l_frame_0;
UINT16 *vboy_font0;
UINT16 *vboy_l_frame_1;
UINT16 *vboy_font1;
UINT16 *vboy_r_frame_0;
UINT16 *vboy_font2;
UINT16 *vboy_r_frame_1;
UINT16 *vboy_font3;
UINT16 *vboy_bg_map;
UINT16 *vboy_paramtab;
UINT16 *vboy_world;
UINT16 *vboy_columntab1;
UINT16 *vboy_columntab2;
UINT16 *vboy_objects;

struct VBOY_REGS_STRUCT
{
	UINT32 lpc, lpc2, lpt, lpr;
	UINT32 khb, klb;
	UINT32 thb, tlb;
	UINT32 tcr, wcr, kcr;
};

struct VBOY_REGS_STRUCT vboy_regs;

READ32_HANDLER( port_02_read )
{
	UINT32 value = 0x00;

	switch (offset & 0xff)
	{
		case 0x10:	// KLB (Keypad Low Byte)
			value = vboy_regs.klb | 0x02;	// 0x02 is always 1
			break;
		case 0x14:	// KHB (Keypad High Byte)
			value = vboy_regs.khb;
			break;
		case 0x20:	// TCR (Timer Control Reg)
			value = vboy_regs.tcr;
			break;
		case 0x24:	// WCR (Wait State Control Reg)
			value = vboy_regs.wcr;
			break;
		case 0x28:	// KCR (Keypad Control Reg)
			value = vboy_regs.kcr;
			break;
		case 0x00:	// LPC (Link Port Control Reg)
		case 0x04:	// LPC2 (Link Port Control Reg)
		case 0x08:	// LPT (Link Port Transmit)
		case 0x0c:	// LPR (Link Port Receive)
		case 0x18:	// TLB (Timer Low Byte)
		case 0x1c:	// THB (Timer High Byte)
		default:
			logerror("Unemulated read: offset %02x\n", offset);
			break;
	}
	return value;
}

WRITE32_HANDLER( port_02_write )
{
	switch (offset)
	{
		case 0x0c:	// LPR (Link Port Receive)
		case 0x10:	// KLB (Keypad Low Byte)
		case 0x14:	// KHB (Keypad High Byte)
			logerror("Ilegal write: offset %02x should be only read\n", offset);
			break;
		case 0x20:	// TCR (Timer Control Reg)
			vboy_regs.kcr = (data | 0xe0) & 0xfd;	// according to docs: bits 5, 6 & 7 are unused and set to 1, bit 1 is read only.
			break;
		case 0x24:	// WCR (Wait State Control Reg)
			vboy_regs.wcr = data | 0xfc;	// according to docs: bits 2 to 7 are unused and set to 1.
			break;
		case 0x28:	// KCR (Keypad Control Reg)
			if (data & 0x04 )
			{
				vboy_regs.klb = (data & 0x01) ? 0 : (input_port_read(space->machine, "INPUT") & 0x00ff);
				vboy_regs.klb = (data & 0x01) ? 0 : (input_port_read(space->machine, "INPUT") & 0xff00) >> 8;
			}
			else if (data & 0x20)
			{
				vboy_regs.klb = input_port_read(space->machine, "INPUT") & 0x00ff;
				vboy_regs.klb = (input_port_read(space->machine, "INPUT") & 0xff00) >> 8;
			}
			vboy_regs.kcr = (data | 0x48) & 0xfd;	// according to docs: bit 6 & bit 3 are unused and set to 1, bit 1 is read only.
			break;
		case 0x00:	// LPC (Link Port Control Reg)
		case 0x04:	// LPC2 (Link Port Control Reg)
		case 0x08:	// LPT (Link Port Transmit)
		case 0x18:	// TLB (Timer Low Byte)
		case 0x1c:	// THB (Timer High Byte)
		default:
			logerror("Unemulated write: offset %02x, data %04x\n", offset, data);
			break;
	}
}

static ADDRESS_MAP_START( vboy_mem, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x07ffffff)
	AM_RANGE( 0x00000000, 0x00005fff ) AM_RAM AM_BASE((UINT32**)&vboy_l_frame_0) // L frame buffer 0
	AM_RANGE( 0x00006000, 0x00007fff ) AM_RAM AM_SHARE(1) AM_BASE((UINT32**)&vboy_font0) // Font 0-511
	AM_RANGE( 0x00008000, 0x0000dfff ) AM_RAM AM_BASE((UINT32**)&vboy_l_frame_1) // L frame buffer 1
	AM_RANGE( 0x0000e000, 0x0000ffff ) AM_RAM AM_SHARE(2) AM_BASE((UINT32**)&vboy_font1) // Font 512-1023
	AM_RANGE( 0x00010000, 0x00015fff ) AM_RAM AM_BASE((UINT32**)&vboy_r_frame_0) // R frame buffer 0
	AM_RANGE( 0x00016000, 0x00017fff ) AM_RAM AM_SHARE(3) AM_BASE((UINT32**)&vboy_font2) // Font 1024-1535
	AM_RANGE( 0x00018000, 0x0001dfff ) AM_RAM AM_BASE((UINT32**)&vboy_r_frame_1) // R frame buffer 1
	AM_RANGE( 0x0001e000, 0x0001ffff ) AM_RAM AM_SHARE(4) AM_BASE((UINT32**)&vboy_font3) // Font 1536-2047
	
	AM_RANGE( 0x00020000, 0x0003bfff ) AM_RAM AM_BASE((UINT32**)&vboy_bg_map) // BG Map
	AM_RANGE( 0x0003c000, 0x0003d7ff ) AM_RAM AM_BASE((UINT32**)&vboy_paramtab) // Param Table
	AM_RANGE( 0x0003d800, 0x0003dbff ) AM_RAM AM_BASE((UINT32**)&vboy_world) // World 
	AM_RANGE( 0x0003dc00, 0x0003ddff ) AM_RAM AM_BASE((UINT32**)&vboy_columntab1) // Column Table 1
	AM_RANGE( 0x0003de00, 0x0003dfff ) AM_RAM AM_BASE((UINT32**)&vboy_columntab2) // Column Table 2
	AM_RANGE( 0x0003e000, 0x0003ffff ) AM_RAM AM_BASE((UINT32**)&vboy_objects) // Objects RAM
	
	AM_RANGE( 0x00040000, 0x0005ffff ) AM_RAM // VIPC	
	
	AM_RANGE( 0x00078000, 0x00079fff ) AM_RAM AM_SHARE(1) // Font 0-511 mirror
	AM_RANGE( 0x0007a000, 0x0007bfff ) AM_RAM AM_SHARE(2) // Font 512-1023 mirror
	AM_RANGE( 0x0007c000, 0x0007dfff ) AM_RAM AM_SHARE(3) // Font 1024-1535 mirror
	AM_RANGE( 0x0007e000, 0x0007ffff ) AM_RAM AM_SHARE(4) // Font 1536-2047 mirror
	
	AM_RANGE( 0x01000000, 0x010005ff ) AM_RAM // Sound RAM 
	AM_RANGE( 0x02000000, 0x0200002b ) AM_MIRROR(0x0ffff00) AM_READWRITE(port_02_read, port_02_write) // Hardware control registers mask 0xff
	//AM_RANGE( 0x04000000, 0x04ffffff ) // Expansion area
	AM_RANGE( 0x05000000, 0x0500ffff ) AM_MIRROR(0x0ff0000) AM_RAM // Main RAM - 64K mask 0xffff
	AM_RANGE( 0x06000000, 0x06003fff ) AM_RAM // Cart RAM - 8K
	AM_RANGE( 0x07000000, 0x071fffff ) AM_MIRROR(0x0e00000) AM_ROM AM_REGION("user1", 0) /* ROM */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vboy )
	PORT_START("INPUT")
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT ) PORT_PLAYER(1)	
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT ) PORT_PLAYER(1)		
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("L") PORT_PLAYER(1) // Left button on back
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("R") PORT_PLAYER(1) // Right button on back
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("A") PORT_PLAYER(1) // A button (or B?)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("B") PORT_PLAYER(1) // B button (or A?)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNUSED ) // Always 1
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNUSED ) // Battery low
INPUT_PORTS_END


static MACHINE_RESET(vboy) 
{
	/* Initial values taken from Reality Boy, to be verified when emulation improves */
	vboy_regs.lpc = 0x6d;
	vboy_regs.lpc2 = 0xff;
	vboy_regs.lpt = 0x00; 
	vboy_regs.lpr = 0x00;
	vboy_regs.klb = 0x00;
	vboy_regs.khb = 0x00;
	vboy_regs.tlb = 0xff;
	vboy_regs.thb = 0xff;
	vboy_regs.tcr = 0xe4;
	vboy_regs.wcr = 0xfc;
	vboy_regs.kcr = 0x4c;
}

static bitmap_t *bg_map[14];

static VIDEO_START( vboy )
{
	int i;
	// Allocate memory for temporary screens
	for(i = 0; i < 14; i++) 
	{
		bg_map[i] = auto_bitmap_alloc(machine, 512, 512, BITMAP_FORMAT_INDEXED16);
	}
}

static void put_char(int x, int y, UINT16 ch, bitmap_t *bitmap)
{
	UINT16 *font = vboy_font0;
	UINT16 code = ch;
	int i, b;
	// determine font buffer from char code used
	if (ch >= 512)  { font = vboy_font1; code = ch - 512;  }
	if (ch >= 1024) { font = vboy_font2; code = ch - 1024; }
	if (ch >= 1536) { font = vboy_font3; code = ch - 1536; }
	
	for(i = 0; i < 8; i++) 
	{
		UINT16  data = font[code * 8 + i]; 
		for (b = 0; b < 8; b++)
		{					
			*BITMAP_ADDR16(bitmap, y + i, x + b) =  ((data >> (b << 1)) & 0x03);
		}		
	}	
}

static void fill_bg_map(int num, bitmap_t *bitmap)
{
	int i, j;	
	// Fill background map
	for (i = 0; i < 64; i++) 
	{
		for (j = 0; j < 64; j++) 
		{
			UINT16 ch = vboy_bg_map[j + 64 * i + (num * 0x1000)];
			put_char(j * 8, i * 8, ch & 0x7ff, bitmap);
		}
	}
}

static UINT8 display_world(int num, bitmap_t *bitmap, UINT8 right)
{
	UINT16 def = vboy_world[num*16];
	UINT16 gx  = vboy_world[num*16+1];
	UINT16 gp  = vboy_world[num*16+2];
	UINT16 gy  = vboy_world[num*16+3];
	UINT16 mx  = vboy_world[num*16+4];
	UINT16 mp  = vboy_world[num*16+5];
	UINT16 my  = vboy_world[num*16+6];
	//UINT16 w  = vboy_world[num*16+7];
	//UINT16 h = vboy_world[num*16+8];
//	UINT16 param_base  = vboy_world[num*16+9];
//	UINT16 overplane = vboy_world[num*16+10];
	UINT8 bg_map_num = def & 0x0f;
	
	if (bg_map_num < 15) {
		fill_bg_map(bg_map_num,bg_map[bg_map_num]);	
		if (BIT(def,15) && (right==0)) {
			// Left screen 
			copybitmap(bitmap,bg_map[bg_map_num],mx-mp,my,gx-gp,gy,NULL);
		}
		if (BIT(def,14) && (right==1)) {
			// Right screen 
			copybitmap(bitmap,bg_map[bg_map_num],mx+mp,my,gx+gp,gy,NULL);
		}
	}
	// Return END world status
	return BIT(def,6);
}

static VIDEO_UPDATE( vboy )
{
	int i;
	UINT8 right = 0;
	const device_config *_3d_right_screen = devtag_get_device(screen->machine, "3dright");
	if (screen == _3d_right_screen) right = 1;

	for(i=31;i>=0;i--) {
		if (display_world(i,bitmap,right)) break;
	}
	return 0;
}

static const rgb_t vboy_palette[18] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x55, 0x00, 0x00), // 1
	MAKE_RGB(0x7f, 0x00, 0x00), // 2 
	MAKE_RGB(0xff, 0x00, 0x00)  // 3  
};

PALETTE_INIT( vboy )
{
	palette_set_colors(machine, 0, vboy_palette, ARRAY_LENGTH(vboy_palette));
}


static MACHINE_DRIVER_START( vboy )
	/* basic machine hardware */
	MDRV_CPU_ADD( "maincpu", V810, XTAL_20MHz )
	MDRV_CPU_PROGRAM_MAP(vboy_mem)

	MDRV_MACHINE_RESET(vboy)

	/* video hardware */    
	MDRV_DEFAULT_LAYOUT(layout_vboy)   

	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(vboy)

	MDRV_VIDEO_START(vboy)
	MDRV_VIDEO_UPDATE(vboy)
     
	/* Left screen */
	MDRV_SCREEN_ADD("3dleft", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50.2)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(384, 224)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)

	/* Right screen */
	MDRV_SCREEN_ADD("3dright", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50.2)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(384, 224)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("vb")
	MDRV_CARTSLOT_MANDATORY
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vboy)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vboy )
	ROM_REGION( 0x200000, "user1", 0 )
	ROM_CART_LOAD("cart", 0x0000, 0x200000, ROM_MIRROR)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   	FULLNAME       FLAGS */
CONS( 1995, vboy,   0,      0, 		 vboy, 		vboy, 	 0,  	 vboy, 	"Nintendo", "Virtual Boy", GAME_NOT_WORKING )
