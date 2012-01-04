/***************************************************************************
   
        DEC Rainbow 100

        04/01/2012 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN
#include "emu.h"
#include "cpu/i86/i86.h"
#include "cpu/z80/z80.h"
#include "video/vtvideo.h"

class rainbow_state : public driver_device
{
public:
	rainbow_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),			
			m_crtc(*this, "vt100_video")
			{ }
	required_device<device_t> m_crtc;
	UINT8 *m_p_ram;
	
	DECLARE_READ8_MEMBER(read_video_ram_r);
	DECLARE_WRITE8_MEMBER(clear_video_interrupt);
};


static ADDRESS_MAP_START( rainbow8088_map, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xec000, 0xedfff) AM_RAM
	AM_RANGE(0xee000, 0xeffff) AM_RAM AM_BASE(m_p_ram)	
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbow8088_io , AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	// 0x04 Video processor DC011
	AM_RANGE (0x04, 0x04) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc011_w)
	// 0x0C Video processor DC012
	AM_RANGE (0x0c, 0x0c) AM_DEVWRITE_LEGACY("vt100_video", vt_video_dc012_w)	
ADDRESS_MAP_END

static ADDRESS_MAP_START(rainbowz80_mem, AS_PROGRAM, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rainbowz80_io , AS_IO, 8, rainbow_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( rainbow )
INPUT_PORTS_END


static MACHINE_RESET(rainbow) 
{	
}

static SCREEN_UPDATE( rainbow )
{
    device_t *devconf = screen.machine().device("vt100_video");
	rainbow_video_update( devconf, bitmap, cliprect);
	return 0;
}

READ8_MEMBER( rainbow_state::read_video_ram_r )
{
	return m_p_ram[offset];
}

WRITE8_MEMBER( rainbow_state::clear_video_interrupt )
{
}

static const vt_video_interface video_interface =
{
	"screen",
	"chargen",
	DEVCB_DRIVER_MEMBER(rainbow_state, read_video_ram_r),
	DEVCB_DRIVER_MEMBER(rainbow_state, clear_video_interrupt)
};

static MACHINE_CONFIG_START( rainbow, rainbow_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8088, 4772720)
    MCFG_CPU_PROGRAM_MAP(rainbow8088_map)
    MCFG_CPU_IO_MAP(rainbow8088_io)	

    MCFG_CPU_ADD("subcpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(rainbowz80_mem)
    MCFG_CPU_IO_MAP(rainbowz80_io)
	MCFG_DEVICE_DISABLE()	

    MCFG_MACHINE_RESET(rainbow)
	
	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*10, 25*10)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*10-1, 0, 25*10-1)
	MCFG_SCREEN_UPDATE(rainbow)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	MCFG_VT100_VIDEO_ADD("vt100_video", video_interface)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rainbow )
    ROM_REGION(0x100000,"maincpu", 0)
	ROM_LOAD( "rblow.16k",  0xf0000, 0x4000, CRC(9d1332b4) SHA1(736306d2a36bd44f95a39b36ebbab211cc8fea6e))
	ROM_RELOAD(0xf4000,0x4000)	
	ROM_LOAD( "rbhigh.16k", 0xf8000, 0x4000, CRC(8638712f) SHA1(8269b0d95dc6efbe67d500dac3999df4838625d8))
	ROM_RELOAD(0xfc000,0x4000)	
	ROM_REGION(0x1000, "chargen", 0)
	// Taken from VT100 but it's 4KB by documentation
	ROM_LOAD( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, rainbow,  0,       0, 	rainbow, 	rainbow, 	 0,  "Digital Equipment Corporation",   "Rainbow 100B",		GAME_NOT_WORKING | GAME_NO_SOUND)

