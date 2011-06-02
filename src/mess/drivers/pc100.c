/***************************************************************************

    NEC PC-100

    TODO:
    - kanji rom has offsetted data for whatever reason.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/i8255.h"


class pc100_state : public driver_device
{
public:
	pc100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT16 *m_kanji_rom;
	UINT16 *m_vram;
	UINT16 m_kanji_addr;

	UINT8 m_bank_r,m_bank_w;
};

static VIDEO_START( pc100 )
{
}

static SCREEN_UPDATE( pc100 )
{
	pc100_state *state = screen->machine().driver_data<pc100_state>();
	int x,y;
	int count;
	int xi;
	int dot;
	int pen[4],pen_i;

	count = 0;

	for(y=0;y<512;y++)
	{
		for(x=0;x<1024/16;x++)
		{
			for(xi=0;xi<16;xi++)
			{
				for(pen_i=0;pen_i<4;pen_i++)
					pen[pen_i] = (state->m_vram[count+pen_i*0x10000] >> xi) & 1;

				dot = 0;
				for(pen_i=0;pen_i<4;pen_i++)
					dot |= pen[pen_i]<<pen_i;

				if(y < 512 && x*16+xi < 768) /* TODO: safety check */
					*BITMAP_ADDR16(bitmap, y, x*16+xi) = screen->machine().pens[dot];
			}

			count++;
		}
	}

	return 0;
}

static READ16_HANDLER( pc100_vram_r )
{
	pc100_state *state = space->machine().driver_data<pc100_state>();

	return state->m_vram[offset+state->m_bank_r*0x10000];
}

static WRITE16_HANDLER( pc100_vram_w )
{
	pc100_state *state = space->machine().driver_data<pc100_state>();
	int i;

	for(i=0;i<4;i++)
	{
		if((state->m_bank_w >> i) & 1)
			COMBINE_DATA(&state->m_vram[offset+i*0x10000]);
	}
}

static ADDRESS_MAP_START(pc100_map, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xbffff) AM_RAM // work ram
	AM_RANGE(0xc0000,0xdffff) AM_READWRITE(pc100_vram_r,pc100_vram_w) // vram, blitter based!
	AM_RANGE(0xf8000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

static READ16_HANDLER( pc100_kanji_r )
{
	pc100_state *state = space->machine().driver_data<pc100_state>();

	return state->m_kanji_rom[state->m_kanji_addr];
}


static WRITE16_HANDLER( pc100_kanji_w )
{
	pc100_state *state = space->machine().driver_data<pc100_state>();

	COMBINE_DATA(&state->m_kanji_addr);
}

static READ8_HANDLER( pc100_key_r )
{
	if(offset)
		return 0x2d; // bit 5: horizontal/vertical monitor dsw

	return 0;
}

/* everything is 8-bit bus wide */
static ADDRESS_MAP_START(pc100_io, AS_IO, 16)
//	AM_RANGE(0x00, 0x03) i8259
//	AM_RANGE(0x04, 0x07) i8237?
//	AM_RANGE(0x08, 0x0b) upd765
//	AM_RANGE(0x10, 0x17) i8255 #1
	AM_RANGE(0x18, 0x1f) AM_DEVREADWRITE8_MODERN("ppi8255_2", i8255_device, read, write,0x00ff) // i8255 #2
	AM_RANGE(0x20, 0x23) AM_READ8(pc100_key_r,0x00ff) //i/o, keyboard, mouse
//	AM_RANGE(0x28, 0x2b) i8251
//	AM_RANGE(0x30, 0x31) crtc shift
//	AM_RANGE(0x38, 0x39) crtc address reg
//	AM_RANGE(0x3a, 0x3b) crtc data reg
//	AM_RANGE(0x3c, 0x3f) crtc vertical start position
//	AM_RANGE(0x40, 0x5f) palette
//	AM_RANGE(0x60, 0x61) crtc command (16-bit wide)
	AM_RANGE(0x80, 0x81) AM_READWRITE(pc100_kanji_r,pc100_kanji_w)
//	AM_RANGE(0x84, 0x87) kanji "strobe" signal 0/1
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( pc100 )
INPUT_PORTS_END

static const gfx_layout kanji_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 },
	{ STEP16(0,16) },
	16*16
};

static GFXDECODE_START( pc100 )
	GFXDECODE_ENTRY( "kanji", 0x0000, kanji_layout, 0, 1 )
GFXDECODE_END

static MACHINE_START(pc100)
{
	pc100_state *state = machine.driver_data<pc100_state>();

	state->m_kanji_rom = (UINT16 *)machine.region("kanji")->base();
	state->m_vram = (UINT16 *)machine.region("vram")->base();
}

static MACHINE_RESET(pc100)
{
}

static WRITE8_DEVICE_HANDLER( lower_mask_w )
{
	//printf("%02x L\n",data);
}

static WRITE8_DEVICE_HANDLER( upper_mask_w )
{
	//printf("%02x H\n",data);
}

static WRITE8_DEVICE_HANDLER( crtc_bank_w )
{
	pc100_state *state = device->machine().driver_data<pc100_state>();

	state->m_bank_w = data & 0xf;
	state->m_bank_r = (data & 0x30) >> 4;
}

static I8255A_INTERFACE( pc100_ppi8255_interface_2 )
{
	DEVCB_NULL,
	DEVCB_HANDLER(lower_mask_w),
	DEVCB_NULL,
	DEVCB_HANDLER(upper_mask_w),
	DEVCB_NULL,
	DEVCB_HANDLER(crtc_bank_w)
};

static MACHINE_CONFIG_START( pc100, pc100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 4000000)
	MCFG_CPU_PROGRAM_MAP(pc100_map)
	MCFG_CPU_IO_MAP(pc100_io)

	MCFG_MACHINE_START(pc100)
	MCFG_MACHINE_RESET(pc100)

    MCFG_I8255_ADD( "ppi8255_2", pc100_ppi8255_interface_2 )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(768, 512)
	MCFG_SCREEN_VISIBLE_AREA(0, 768-1, 0, 512-1)
	MCFG_SCREEN_UPDATE(pc100)

	MCFG_PALETTE_LENGTH(16)
//	MCFG_PALETTE_INIT(black_and_white)

	MCFG_GFXDECODE(pc100)

	MCFG_VIDEO_START(pc100)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pc100 )
	ROM_REGION( 0x8000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x8000, CRC(fd54a80e) SHA1(605a1b598e623ba2908a14a82454b9d32ea3c331))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, BAD_DUMP CRC(cb63802a) SHA1(0f9c2cd11b41b46a5c51426a04376c232ec90448))

	ROM_REGION( 0x20000*4, "vram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, pc100,  0,      0,       pc100,     pc100,    0,     "NEC",   "PC-100", GAME_NOT_WORKING | GAME_NO_SOUND)
