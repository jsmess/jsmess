/***************************************************************************
Video Technology Genius computers:
    VTech Genius PC (France)
    VTech Genius IQ 512 (Germany)
    The French packaging mentions distributions in Switzerland, the Netherlands,
    USA, Canada, and UK as well. Looking for more information and ROM dumps.

System driver:

    Adrien Destugues <pulkomandy@gmail.com>, May 2012
      - First attempt

Memory map:
    00000000 System ROM (2MB)
    00200000 RAM (256K)
    00400000 Flash memory (128K)
    00600000 Some memory mapped hardware
    ???????? Cartridge port

TODO:
    - Mostly everything besides CPU, RAM and ROM
    - Check with different countries ROMs

Not very much is known about this computer released in 1997.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/intelfsh.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class geniusiq_state : public driver_device
{
public:
	geniusiq_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_flash(*this, "flash"),
		m_vram(*this, "vram"),
		m_mouse_gfx(*this, "mouse_gfx")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<intelfsh8_device> m_flash;
	required_shared_ptr<UINT16>	m_vram;
	required_shared_ptr<UINT16>	m_mouse_gfx;
	virtual void machine_reset();
	virtual UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_READ8_MEMBER(flash_r);
	DECLARE_WRITE8_MEMBER(flash_w);
	DECLARE_READ16_MEMBER(mouse_r);
	DECLARE_WRITE16_MEMBER(mouse_pos_w);

	DECLARE_READ16_MEMBER(unk0_r) { return 0; }
	DECLARE_READ16_MEMBER(unk_r) { return machine().rand(); }

	UINT16		m_mouse_posx;
	UINT16		m_mouse_posy;
};


UINT32 geniusiq_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	for (int y=0; y<256; y++)
		for (int x=0; x<256; x+=2)
		{
			UINT16 data = m_vram[(y*256 + x)>>1];

			for(int b=0; b<4; b++)
			{
				bitmap.pix16(y, x*2 + b) = (data>>12) & 0x0f;
				data <<= 4;
			}
		}

	// mouse cursor
	for (int y=0; y<16; y++)
		for (int x=0; x<6; x+=2)
		{
			UINT16 data = m_mouse_gfx[(y*6 + x)>>1];

			for(int b=0; b<4; b++)
			{
				UINT8 pen = (data>>12) & 0x0f;

				// I assume color 0 is transparent
				if(pen != 0 && screen.visible_area().contains(m_mouse_posx + x*2 + b, m_mouse_posy + y))
					bitmap.pix16(m_mouse_posy + y, m_mouse_posx + x*2 + b) = pen;
				data <<= 4;
			}
		}

	return 0;
}

READ8_MEMBER(geniusiq_state::flash_r)
{
	return m_flash->read(offset);
}

WRITE8_MEMBER(geniusiq_state::flash_w)
{
	m_flash->write(offset, data);
}

WRITE16_MEMBER( geniusiq_state::mouse_pos_w )
{
	switch (offset)
	{
		case 0:		m_mouse_posx = data;	break;
		case 1:		m_mouse_posy = data;	break;
	}
}

READ16_MEMBER( geniusiq_state::mouse_r )
{
	return 0;
}


static ADDRESS_MAP_START(geniusiq_mem, AS_PROGRAM, 16, geniusiq_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x1FFFFF) AM_ROM
	AM_RANGE(0x200000, 0x23FFFF) AM_RAM
	AM_RANGE(0x300000, 0x30FFFF) AM_RAM		AM_SHARE("vram")
	AM_RANGE(0x310000, 0x31FFFF) AM_RAM
	AM_RANGE(0x400000, 0x41ffff) AM_MIRROR(0x0e0000) AM_READWRITE8(flash_r, flash_w, 0x00ff)
	AM_RANGE(0x600300, 0x600301) AM_READ(mouse_r)
	//AM_RANGE(0x600500, 0x60050f)                      // read during IRQ 5 (keyboard ??)
	AM_RANGE(0x600918, 0x600919) AM_READ(unk0_r)        // loop at start if bit 0 is set
	AM_RANGE(0x601008, 0x601009) AM_READ(unk_r)			// unknown, read at start and expect that bit 2 changes several times before continue
	AM_RANGE(0x601010, 0x601011) AM_READ(unk0_r)		// loop at start if bit 1 is set
	AM_RANGE(0x601060, 0x601063) AM_WRITE(mouse_pos_w)
	AM_RANGE(0x601100, 0x6011ff) AM_RAM		AM_SHARE("mouse_gfx")	// mouse cursor gfx (12x16)
	// 0x600000 : some memory mapped hardware
	// Somewhere : cartridge port
ADDRESS_MAP_END


static INPUT_CHANGED( trigger_irq )
{
	cputag_set_input_line(field.machine(), "maincpu", (int)(FPTR)param, newval ? HOLD_LINE : CLEAR_LINE);
}

/* Input ports */
static INPUT_PORTS_START( geniusiq )
	PORT_START( "DEBUG" )	// for debug purposes, to be removed in the end
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_1 ) PORT_NAME( "IRQ 1" ) PORT_CHANGED( trigger_irq, 1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_2 ) PORT_NAME( "IRQ 2" ) PORT_CHANGED( trigger_irq, 2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_3 ) PORT_NAME( "IRQ 3" ) PORT_CHANGED( trigger_irq, 3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_4 ) PORT_NAME( "IRQ 4" ) PORT_CHANGED( trigger_irq, 4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_5 ) PORT_NAME( "IRQ 5" ) PORT_CHANGED( trigger_irq, 5 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_6 ) PORT_NAME( "IRQ 6" ) PORT_CHANGED( trigger_irq, 6 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_7 ) PORT_NAME( "IRQ 7" ) PORT_CHANGED( trigger_irq, 7 )
INPUT_PORTS_END


MACHINE_RESET_MEMBER( geniusiq_state )
{
}

static MACHINE_CONFIG_START( geniusiq, geniusiq_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 16000000) // The main crystal is at 32MHz, not sure whats the CPU freq
	MCFG_CPU_PROGRAM_MAP(geniusiq_mem)
	MCFG_CPU_VBLANK_INT("screen", irq6_line_hold)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(512, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MCFG_SCREEN_UPDATE_DRIVER( geniusiq_state, screen_update )
	MCFG_PALETTE_LENGTH(16)

	/* internal flash */
	MCFG_AMD_29F010_ADD("flash")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( geniusiq )
	ROM_REGION(0x200000, "maincpu", 0)
	ROM_LOAD( "geniusiq.bin", 0x0000, 0x200000, CRC(9b06cbf1) SHA1(b9438494a9575f78117c0033761f899e3c14e292) )
ROM_END

ROM_START( geniusiq_de )
	ROM_REGION(0x200000, "maincpu", 0)
	ROM_LOAD( "german.rom", 0x0000, 0x200000, CRC(a98fc3ff) SHA1(de76a5898182bd0180bd2b3e34c4502f0918a3fa) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                FULLNAME               FLAGS */
COMP( 1997, geniusiq,      0,   		   0,    geniusiq,   geniusiq,  0,  "Video Technology", "Genius IQ 128 (France)", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1997, geniusiq_de,   geniusiq,       0,    geniusiq,   geniusiq,  0,  "Video Technology", "Genius IQ 128 (Germany)", GAME_NOT_WORKING | GAME_NO_SOUND)
