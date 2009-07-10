/***************************************************************************
   
        Nintendo Virtual Boy

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/v810/v810.h"
#include "devices/cartslot.h"

UINT32 *vboy_l_frame_0;
UINT32 *vboy_font0;
UINT32 *vboy_l_frame_1;
UINT32 *vboy_font1;
UINT32 *vboy_r_frame_0;
UINT32 *vboy_font2;
UINT32 *vboy_r_frame_1;
UINT32 *vboy_font3;
UINT32 *vboy_bg_map;
UINT32 *vboy_paramtab;
UINT32 *vboy_world;
UINT32 *vboy_columntab1;
UINT32 *vboy_columntab2;
UINT32 *vboy_objects;
	
static ADDRESS_MAP_START( vboy_mem, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x07FFFFFF)
	AM_RANGE( 0x00000000, 0x00005FFF ) AM_RAM AM_BASE(&vboy_l_frame_0) // L Frame buffer 0
	AM_RANGE( 0x00006000, 0x00007FFF ) AM_RAM AM_SHARE(1) AM_BASE(&vboy_font0) // Font 0-511
	AM_RANGE( 0x00008000, 0x0000DFFF ) AM_RAM AM_BASE(&vboy_l_frame_1) // L Frame buffer 1
	AM_RANGE( 0x0000E000, 0x0000FFFF ) AM_RAM AM_SHARE(2) AM_BASE(&vboy_font1) // Font 512-1023
	AM_RANGE( 0x00010000, 0x00015FFF ) AM_RAM AM_BASE(&vboy_r_frame_0) // R Frame buffer 0
	AM_RANGE( 0x00016000, 0x00017FFF ) AM_RAM AM_SHARE(3) AM_BASE(&vboy_font2) // Font 1024-1535
	AM_RANGE( 0x00018000, 0x0001DFFF ) AM_RAM AM_BASE(&vboy_r_frame_1) // R Frame buffer 1
	AM_RANGE( 0x0001E000, 0x0001FFFF ) AM_RAM AM_SHARE(4) AM_BASE(&vboy_font3) // Font 1536-2047
	
	AM_RANGE( 0x00020000, 0x0003BFFF ) AM_RAM AM_BASE(&vboy_bg_map) // BG Map
	AM_RANGE( 0x0003C000, 0x0003D7FF ) AM_RAM AM_BASE(&vboy_paramtab) // Param Table
	AM_RANGE( 0x0003D800, 0x0003DBFF ) AM_RAM AM_BASE(&vboy_world) // World 
	AM_RANGE( 0x0003DC00, 0x0003DDFF ) AM_RAM AM_BASE(&vboy_columntab1) // Column Table 1
	AM_RANGE( 0x0003DE00, 0x0003DFFF ) AM_RAM AM_BASE(&vboy_columntab2) // Column Table 2
	AM_RANGE( 0x0003E000, 0x0003FFFF ) AM_RAM AM_BASE(&vboy_objects) // Objects RAM
	
	AM_RANGE( 0x00040000, 0x0005FFFF ) AM_RAM // VIPC	
	
	AM_RANGE( 0x00078000, 0x00079FFF ) AM_RAM AM_SHARE(1) // Font 0-511 mirror
	AM_RANGE( 0x0007A000, 0x0007BFFF ) AM_RAM AM_SHARE(2) // Font 512-1023 mirror
	AM_RANGE( 0x0007C000, 0x0007DFFF ) AM_RAM AM_SHARE(3) // Font 1024-1535 mirror
	AM_RANGE( 0x0007E000, 0x0007FFFF ) AM_RAM AM_SHARE(4) // Font 1536-2047 mirror
	
	AM_RANGE( 0x01000000, 0x010005FF ) AM_RAM // Sound RAM 
	AM_RANGE( 0x02000000, 0x0200002B ) AM_MIRROR(0x0FFFF00) AM_RAM // Hardware control registers mask 0xff
	//AM_RANGE( 0x04000000, 0x04FFFFFF ) // Expansion area
	AM_RANGE( 0x05000000, 0x0500FFFF ) AM_MIRROR(0x0FF0000) AM_RAM // Main RAM - 64K mask 0xffff
	AM_RANGE( 0x06000000, 0x06003FFF ) AM_RAM // Cart RAM - 8K
	AM_RANGE( 0x07000000, 0x071FFFFF ) AM_MIRROR(0x0E00000) AM_ROM AM_REGION("user1", 0) /* ROM */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vboy )
INPUT_PORTS_END


static MACHINE_RESET(vboy) 
{
}

static VIDEO_START( vboy )
{
}

static void display_char(int x, int y, UINT16 ch, bitmap_t *bitmap)
{
	UINT32 *font = vboy_font0;
	UINT16 code = ch,t0,t1;
	int i,b;
	if (ch >=512)  { font = vboy_font1; code = ch - 512;  }
	if (ch >=1024) { font = vboy_font2; code = ch - 1024; }
	if (ch >=1536) { font = vboy_font3; code = ch - 1536; }
	
	for(i=0;i<4;i++) {
		t0 = font [code*4+i] & 0xffff; 
		t1 = (font[code*4+i] >> 16) & 0xffff;		
		for (b = 0; b < 8; b++)
		{					
			*BITMAP_ADDR16(bitmap, y+i*2,   x + b) =  ((t0 >> (b*2)) & 0x03);
			*BITMAP_ADDR16(bitmap, y+i*2+1, x + b) =  ((t1 >> (b*2)) & 0x03);
		}		
	}
	
}

static void display_bg_map(int num, bitmap_t *bitmap)
{
	int i,j;
	UINT16 c0,c1;
	for (i=0;i<64;i++) {		
		for (j=0;j<32;j++) {
			c0 = vboy_bg_map[j+32*i] & 0xffff;
			c1 = (vboy_bg_map[j+32*i] >> 16) & 0xffff;			
			display_char(j*16,8*i,c0,bitmap);
			display_char(j*16+8,8*i,c1,bitmap);
		}
	}
}
static VIDEO_UPDATE( vboy )
{
	display_bg_map(0,bitmap);
    return 0;
}
static const rgb_t vboy_palette[18] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x55, 0x55, 0x55), // 1
	MAKE_RGB(0x7f, 0x7f, 0x7f), // 2 
	MAKE_RGB(0xff, 0xff, 0xff)  // 3  
};

PALETTE_INIT( vboy )
{
	palette_set_colors(machine, 0, vboy_palette, ARRAY_LENGTH(vboy_palette));
}


static MACHINE_DRIVER_START( vboy )
    /* basic machine hardware */
    MDRV_CPU_ADD( "maincpu", V810, 21477270 )
    MDRV_CPU_PROGRAM_MAP(vboy_mem)

    MDRV_MACHINE_RESET(vboy)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    //MDRV_SCREEN_SIZE(384, 224)
    //MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)
    MDRV_SCREEN_SIZE(512, 512)
    MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 512-1)
    MDRV_PALETTE_LENGTH(4)
    MDRV_PALETTE_INIT(vboy)

    MDRV_VIDEO_START(vboy)
    MDRV_VIDEO_UPDATE(vboy)
	
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
CONS( 1995, vboy,   0,      0, 		 vboy, 		vboy, 	 0,  	 vboy, 	"Nintendo", "Virtual Boy", GAME_NOT_WORKING)

