/***************************************************************************

    Argo

    16/03/2011 Skeleton driver.

    Using info obtained from EMU-80.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)

class argo_state : public driver_device
{
public:
	argo_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	DECLARE_WRITE8_MEMBER(argo_videoram_w);
	UINT8 *m_p_videoram;
	const UINT8 *m_p_chargen;
	UINT8 m_framecnt;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

WRITE8_MEMBER(argo_state::argo_videoram_w)
{
	UINT8 *RAM = machine().region("videoram")->base();
	RAM[offset] = data;
}

static ADDRESS_MAP_START(argo_mem, AS_PROGRAM, 8, argo_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK("boot")
	AM_RANGE(0x0800, 0xf7af) AM_RAM
	AM_RANGE(0xf7b0, 0xf7ff) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE(0xf800, 0xffff) AM_ROM AM_WRITE(argo_videoram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(argo_io, AS_IO, 8, argo_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00A1, 0x00A1) AM_READ_PORT("X0")
	AM_RANGE(0x01A1, 0x01A1) AM_READ_PORT("X1")
	AM_RANGE(0x02A1, 0x02A1) AM_READ_PORT("X2")
	AM_RANGE(0x03A1, 0x03A1) AM_READ_PORT("X3")
	AM_RANGE(0x04A1, 0x04A1) AM_READ_PORT("X4")
	AM_RANGE(0x05A1, 0x05A1) AM_READ_PORT("X5")
	AM_RANGE(0x06A1, 0x06A1) AM_READ_PORT("X6")
	AM_RANGE(0x07A1, 0x07A1) AM_READ_PORT("X7")
	AM_RANGE(0x08A1, 0x08A1) AM_READ_PORT("X8")
	AM_RANGE(0x09A1, 0x09A1) AM_READ_PORT("X9")
	AM_RANGE(0x0AA1, 0x0AA1) AM_READ_PORT("XA")
	AM_RANGE(0x0BA1, 0x0BA1) AM_READ_PORT("XB")
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( argo )
	PORT_START("X0")
	PORT_START("X1")
	PORT_START("X2")
	PORT_START("X3")
	PORT_START("X4")
	PORT_START("X5")
	PORT_START("X6")
	PORT_START("X7")
	PORT_START("X8")
	PORT_START("X9")
	PORT_START("XA")
	PORT_START("XB")
INPUT_PORTS_END


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( argo_boot )
{
	memory_set_bank(machine, "boot", 0);
}

MACHINE_RESET_MEMBER(argo_state)
{
	memory_set_bank(m_machine, "boot", 1);
	m_machine.scheduler().timer_set(attotime::from_usec(5), FUNC(argo_boot));
}

DRIVER_INIT( argo )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xf800);
}

VIDEO_START_MEMBER( argo_state )
{
	m_p_chargen = m_machine.region("chargen")->base();
}

SCREEN_UPDATE_MEMBER( argo_state )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *vram = m_p_videoram;

	m_framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;

				if (ra < 9)
				{
					chr = vram[x];

					/* Take care of flashing characters */
					if ((chr & 0x80) && (m_framecnt & 0x08))
						chr = 0x20;

					chr &= 0x7f;

					gfx = m_p_chargen[(chr<<4) | ra ];
				}

				/* Display a scanline of a character */
				*p++ = BIT(gfx, 7);
				*p++ = BIT(gfx, 6);
				*p++ = BIT(gfx, 5);
				*p++ = BIT(gfx, 4);
				*p++ = BIT(gfx, 3);
				*p++ = BIT(gfx, 2);
				*p++ = BIT(gfx, 1);
				*p++ = BIT(gfx, 0);
			}
		}

		if (y)
			ma+=80;
		else
		{
			ma=0;
			vram = machine().region("videoram")->base();
		}
	}
	return 0;
}

static MACHINE_CONFIG_START( argo, argo_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 3500000)
	MCFG_CPU_PROGRAM_MAP(argo_mem)
	MCFG_CPU_IO_MAP(argo_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( argo )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "argo.rom", 0xf800, 0x0800, CRC(4c4c045b) SHA1(be2b97728cc190d4a8bd27262ba9423f252d31a3) )

	ROM_REGION( 0x0800, "videoram", ROMREGION_ERASEFF )

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, argo,  0,      0,       argo,     argo,    argo,     "unknown",   "Argo", GAME_NOT_WORKING | GAME_NO_SOUND)
