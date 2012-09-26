/***************************************************************************

    Toshiba Pasopia

    TODO:
    - just like Pasopia 7, z80pio is broken, hence it doesn't clear irqs
      after the first one (0xfe79 is the work ram buffer for keyboard).
    - machine emulation needs merging with Pasopia 7 (video emulation is
      completely different tho)

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
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
	UINT8 m_vram_latch,m_attr_latch;
	UINT8 m_video_wl;
	UINT8 m_gfx_mode;

	UINT8 m_crtc_vreg[0x100],m_crtc_index;
	mc6845_device *m_mc6845;
};

#define mc6845_h_char_total 	(state->m_crtc_vreg[0])
#define mc6845_h_display		(state->m_crtc_vreg[1])
#define mc6845_h_sync_pos		(state->m_crtc_vreg[2])
#define mc6845_sync_width		(state->m_crtc_vreg[3])
#define mc6845_v_char_total		(state->m_crtc_vreg[4])
#define mc6845_v_total_adj		(state->m_crtc_vreg[5])
#define mc6845_v_display		(state->m_crtc_vreg[6])
#define mc6845_v_sync_pos		(state->m_crtc_vreg[7])
#define mc6845_mode_ctrl		(state->m_crtc_vreg[8])
#define mc6845_tile_height		(state->m_crtc_vreg[9]+1)
#define mc6845_cursor_y_start	(state->m_crtc_vreg[0x0a])
#define mc6845_cursor_y_end 	(state->m_crtc_vreg[0x0b])
#define mc6845_start_addr		(((state->m_crtc_vreg[0x0c]<<8) & 0x3f00) | (state->m_crtc_vreg[0x0d] & 0xff))
#define mc6845_cursor_addr  	(((state->m_crtc_vreg[0x0e]<<8) & 0x3f00) | (state->m_crtc_vreg[0x0f] & 0xff))
#define mc6845_light_pen_addr	(((state->m_crtc_vreg[0x10]<<8) & 0x3f00) | (state->m_crtc_vreg[0x11] & 0xff))
#define mc6845_update_addr  	(((state->m_crtc_vreg[0x12]<<8) & 0x3f00) | (state->m_crtc_vreg[0x13] & 0xff))


static VIDEO_START( pasopia )
{
}

static SCREEN_UPDATE( pasopia )
{
	pasopia_state *state = screen->machine().driver_data<pasopia_state>();
	int x,y/*,xi,yi*/;
	int xi,yi;
	UINT8 *gfx_rom = screen->machine().region("font")->base();

	for(y=0;y<mc6845_v_display;y++)
	{
		for(x=0;x<mc6845_h_display;x++)
		{
			int tile = state->m_vram[((x+y*mc6845_h_display) + mc6845_start_addr) & 0x3fff] & 0xff;
			int pen;
			int color = 7;

			for(yi=0;yi<mc6845_tile_height;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? color : -1;

					//if(pen != -1)
						if(y*mc6845_tile_height+yi < screen->machine().primary_screen->visible_area().max_y && x*8+xi < screen->machine().primary_screen->visible_area().max_x) /* TODO: safety check */
							*BITMAP_ADDR16(bitmap, y*mc6845_tile_height+yi, x*8+xi) = screen->machine().pens[pen];
				}
			}
		}
	}

	/* quick and dirty way to do the cursor */
	for(yi=0;yi<mc6845_tile_height;yi++)
	{
		for(xi=0;xi<8;xi++)
		{
			if(mc6845_h_display)
			{
				x = mc6845_cursor_addr % mc6845_h_display;
				y = mc6845_cursor_addr / mc6845_h_display;
				*BITMAP_ADDR16(bitmap, y*mc6845_tile_height+yi, x*8+xi) = screen->machine().pens[7];
			}
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


static WRITE8_HANDLER( pasopia_6845_address_w )
{
	pasopia_state *state = space->machine().driver_data<pasopia_state>();

	state->m_crtc_index = data;
	state->m_mc6845->address_w(*space, offset, data);
}

static WRITE8_HANDLER( pasopia_6845_data_w )
{
	pasopia_state *state = space->machine().driver_data<pasopia_state>();

	state->m_crtc_vreg[state->m_crtc_index] = data;
	state->m_mc6845->register_w(*space, offset, data);
}

static ADDRESS_MAP_START(pasopia_io, AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00,0x03) AM_DEVREADWRITE_MODERN("ppi8255_0", i8255_device, read, write)
	AM_RANGE(0x08,0x0b) AM_DEVREADWRITE_MODERN("ppi8255_1", i8255_device, read, write)
	AM_RANGE(0x10,0x10) AM_WRITE(pasopia_6845_address_w)
	AM_RANGE(0x11,0x11) AM_WRITE(pasopia_6845_data_w)
//  0x18 - 0x1b pac2
	AM_RANGE(0x20,0x23) AM_DEVREADWRITE_MODERN("ppi8255_2", i8255_device, read, write)
	AM_RANGE(0x28,0x2b) AM_DEVREADWRITE("ctc", z80ctc_r,z80ctc_w)
	AM_RANGE(0x30,0x33) AM_DEVREADWRITE("z80pio", z80pio_ba_cd_r, z80pio_cd_ba_w)
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
	state->m_mc6845 = machine.device<mc6845_device>("crtc");
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

static READ8_DEVICE_HANDLER( vram_latch_r )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	return state->m_vram[state->m_vram_addr & 0x3fff];
}

static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_HANDLER(vram_addr_lo_w),		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_HANDLER(vram_latch_w),		/* Port B write */
	DEVCB_HANDLER(vram_latch_r),		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

static READ8_DEVICE_HANDLER( portb_1_r )
{
	/*
    x--- ---- attribute latch
    -x-- ---- hblank
    --x- ---- vblank
    ---x ---- LCD system mode, active low
    */
	pasopia_state *state = device->machine().driver_data<pasopia_state>();
	UINT8 grph_latch,lcd_mode;

	state->m_hblank ^= 0x40; //TODO
	grph_latch = (state->m_vram[(state->m_vram_addr & 0x3fff)+0x4000] & 0x80);
	lcd_mode = 0x10;

	return state->m_hblank | lcd_mode | grph_latch; //bit 4: LCD mode
}


static WRITE8_DEVICE_HANDLER( vram_addr_hi_w )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();
	state->m_attr_latch = (data & 0x80) | (state->m_attr_latch & 0x7f);
	if(data & 0x40 && ((state->m_video_wl & 0x40) == 0))
	{
		state->m_vram[state->m_vram_addr & 0x3fff] = state->m_vram_latch;
		state->m_vram[(state->m_vram_addr & 0x3fff)+0x4000] = state->m_attr_latch;
	}

	state->m_video_wl = data & 0x40;
	state->m_vram_addr = (state->m_vram_addr & 0xff) | ((data & 0x3f) << 8);
}

static WRITE8_DEVICE_HANDLER( screen_mode_w )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	state->m_gfx_mode = (data & 0xe0) >> 5;
	state->m_attr_latch = (state->m_attr_latch & 0x80) | (data & 7);
	printf("%02x\n",data);
}

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_HANDLER(screen_mode_w),		/* Port A write */
	DEVCB_HANDLER(portb_1_r),		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_NULL,		/* Port C read */
	DEVCB_HANDLER(vram_addr_hi_w)		/* Port C write */
};

static READ8_DEVICE_HANDLER( rombank_r )
{
	pasopia_state *state = device->machine().driver_data<pasopia_state>();

	return state->m_ram_bank<<1;
}

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,		/* Port A read */
	DEVCB_NULL,		/* Port A write */
	DEVCB_NULL,		/* Port B read */
	DEVCB_NULL,		/* Port B write */
	DEVCB_HANDLER(rombank_r),		/* Port C read */
	DEVCB_NULL		/* Port C write */
};

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_LINE(z80ctc_trg1_w),		// ZC/TO0 callback
	DEVCB_LINE(z80ctc_trg2_w),		// ZC/TO1 callback, beep interface
	DEVCB_LINE(z80ctc_trg3_w)		// ZC/TO2 callback
};

static READ8_DEVICE_HANDLER( testa_r )
{
	printf("A R\n");
	return 0xff;
}

static READ8_DEVICE_HANDLER( testb_r )
{
//  printf("B R\n");
	return 0xff;
}

static WRITE_LINE_DEVICE_HANDLER( testa_w )
{
	printf("A %02x\n",state);
}

static WRITE_LINE_DEVICE_HANDLER( testb_w )
{
	printf("B %02x\n",state);
}

static Z80PIO_INTERFACE( z80pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), //doesn't work?
	DEVCB_HANDLER(testa_r),
	DEVCB_NULL,
	DEVCB_LINE(testa_w),
	DEVCB_HANDLER(testb_r),
	DEVCB_NULL,
	DEVCB_LINE(testb_w)
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

static const z80_daisy_config pasopia_daisy[] =
{
	{ "ctc" },
	{ "z80pio" },
//  { "upd765" }, /* TODO */
	{ NULL }
};


static MACHINE_CONFIG_START( pasopia, pasopia_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 4000000)
	MCFG_CPU_PROGRAM_MAP(pasopia_map)
	MCFG_CPU_IO_MAP(pasopia_io)
	MCFG_CPU_CONFIG(pasopia_daisy)

	MCFG_MACHINE_START(pasopia)
	MCFG_MACHINE_RESET(pasopia)

	MCFG_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MCFG_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MCFG_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )

	MCFG_Z80CTC_ADD( "ctc", XTAL_4MHz, ctc_intf )
	MCFG_Z80PIO_ADD( "z80pio", XTAL_4MHz, z80pio_intf )

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
//  MCFG_PALETTE_INIT(black_and_white)

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
