/***************************************************************************

	Toshiba Pasopia

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255.h"
#include "video/mc6845.h"


class pasopia_state : public driver_device
{
public:
	pasopia_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 m_ram_bank;
	UINT8 *m_prg_rom;
	UINT8 *m_wram;
	UINT8 *m_vram;
	UINT8 m_hblank;
	UINT16 m_vram_addr;
	UINT8 m_vram_latch;
	UINT8 m_video_wl;
};


static VIDEO_START( pasopia )
{
}

static SCREEN_UPDATE( pasopia )
{
	pasopia_state *state = screen->machine().driver_data<pasopia_state>();
	int x,y/*,xi,yi*/;
	int count;

	count = 0x0000;

	for(y=0;y<24;y++)
	{
		for(x=0;x<37;x++)
		{
			int tile = state->m_vram[count];
			//int attr = vram[count+0x4000];
			//int color = attr & 7;

			drawgfx_transpen(bitmap,cliprect,screen->machine().gfx[0],tile,0,0,0,x*8,y*8,0);

			#if 0
			// draw cursor
			if(state->m_cursor_addr*8 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(state->m_cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine.primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine.primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(state->m_cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8-yc+7, x*8+xc) = machine.pens[0x27];
						}
					}
				}
			}
			#endif
			count++;
		}
	}

	return 0;
}

static READ8_HANDLER( pasopia_romram_r )
{
	pasopia_state *state = space->machine().driver_data<pasopia_state>();

	if(state->m_ram_bank)
		return state->m_wram[offset];

	return state->m_prg_rom[offset];
}

static WRITE8_HANDLER( pasopia_ram_w )
{
	pasopia_state *state = space->machine().driver_data<pasopia_state>();

	state->m_wram[offset] = data;
}

static WRITE8_HANDLER( pasopia_ctrl_w )
{
	pasopia_state *state = space->machine().driver_data<pasopia_state>();

	state->m_ram_bank = data & 2;
}

static ADDRESS_MAP_START(pasopia_map, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x7fff) AM_READWRITE(pasopia_romram_r,pasopia_ram_w)
	AM_RANGE(0x8000,0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pasopia_io, AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00,0x03) AM_DEVREADWRITE_MODERN("ppi8255_0", i8255_device, read, write)
	AM_RANGE(0x08,0x0b) AM_DEVREADWRITE_MODERN("ppi8255_1", i8255_device, read, write)
	AM_RANGE(0x10,0x10) AM_DEVWRITE_MODERN("crtc", mc6845_device, address_w)
	AM_RANGE(0x11,0x11) AM_DEVWRITE_MODERN("crtc", mc6845_device, register_w)
//  0x18 - 0x1b pac2
	AM_RANGE(0x20,0x23) AM_DEVREADWRITE_MODERN("ppi8255_2", i8255_device, read, write)
//	0x28 - 0x2b z80ctc
//  0x30 - 0x33 z80pio
//  0x38 printer
	AM_RANGE(0x3c,0x3c) AM_WRITE(pasopia_ctrl_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pasopia )
INPUT_PORTS_END

static MACHINE_START(pasopia)
{
	pasopia_state *state = machine.driver_data<pasopia_state>();

	state->m_prg_rom = machine.region("ipl")->base();
	state->m_wram = machine.region("wram")->base();
	state->m_vram = machine.region("vram")->base();
}

static MACHINE_RESET(pasopia)
{

}

static WRITE8_DEVICE_HANDLER( vram_addr_lo_w )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	state->m_vram_addr = (state->m_vram_addr & 0xff00) | (data & 0xff);
}

static WRITE8_DEVICE_HANDLER( vram_latch_w )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	state->m_vram_latch = data;
}


static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_HANDLER(vram_addr_lo_w),		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_HANDLER(vram_latch_w),		/* Port B write */
	DEVCB_NULL,		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

static READ8_DEVICE_HANDLER( portb_1_r )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	state->m_hblank ^= 0x40; //TODO

	return state->m_hblank | 0x10; //bit 4: LCD mode
}


static WRITE8_DEVICE_HANDLER( vram_addr_hi_w )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();
	if(data & 0x40 && ((state->m_video_wl & 0x40) == 0))
	{
		state->m_vram[state->m_vram_addr & 0x3fff] = state->m_vram_latch;
	}

	state->m_video_wl = data & 0x40;
	state->m_vram_addr = (state->m_vram_addr & 0xff) | ((data & 0x3f) << 8);
}

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_NULL,		/* Port A write */
	DEVCB_HANDLER(portb_1_r),		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_NULL,		/* Port C read */
	DEVCB_HANDLER(vram_addr_hi_w)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_NULL,		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_NULL,		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static const gfx_layout p7_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( pasopia )
	GFXDECODE_ENTRY( "font",   0x00000, p7_chars_8x8,    0, 0x10 )
GFXDECODE_END

static MACHINE_CONFIG_START( pasopia, pasopia_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 4000000)
	MCFG_CPU_PROGRAM_MAP(pasopia_map)
	MCFG_CPU_IO_MAP(pasopia_io)

	MCFG_MACHINE_START(pasopia)
	MCFG_MACHINE_RESET(pasopia)

	MCFG_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MCFG_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MCFG_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(pasopia)
	MCFG_GFXDECODE(pasopia)

	MCFG_MC6845_ADD("crtc", H46505, XTAL_4MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

	MCFG_PALETTE_LENGTH(8)
//	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(pasopia)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pasopia )
	ROM_REGION( 0x8000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "tbasic.rom", 0x0000, 0x8000, CRC(f53774ff) SHA1(bbec45a3bad8d184505cc6fe1f6e2e60a7fb53f2))

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, BAD_DUMP CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412)) //stolen from pasopia7

	ROM_REGION( 0x8000, "wram", ROMREGION_ERASE00 )

	ROM_REGION( 0x8000, "vram", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, pasopia,  0,      0,       pasopia,     pasopia,    0,     "Toshiba",   "Pasopia", GAME_NOT_WORKING | GAME_NO_SOUND)
