/***************************************************************************

	Toshiba Pasopia 1600

	TODO:
	- charset ROM is WRONG! (needs a 8x16 or even a 16x16 one)
	- identify fdc type (needs a working floppy image)

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/pic8259.h"
#include "machine/8237dma.h"


class paso1600_state : public driver_device
{
public:
	paso1600_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		{ }

	UINT8 m_crtc_vreg[0x100],m_crtc_index;

	mc6845_device *m_mc6845;
	UINT8 *m_char_rom;
	UINT16 *m_vram;
	UINT16 *m_gvram;

	struct{
		UINT8 portb;
	}m_keyb;
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


static VIDEO_START( paso1600 )
{
}

static SCREEN_UPDATE( paso1600 )
{
	paso1600_state *state = screen->machine().driver_data<paso1600_state>();
	int x,y;
	int xi,yi;
	UINT8 *gfx_rom = screen->machine().region("font")->base();
	#if 0
	UINT32 count;
	static int test_x;

	if(screen->machine().input().code_pressed(KEYCODE_Z))
		test_x++;

	if(screen->machine().input().code_pressed(KEYCODE_X))
		test_x--;

	popmessage("%d",test_x);

	count = 0;

	for(y=0;y<475;y++)
	{
		count &= 0xffff;

		for(x=0;x<test_x/16;x++)
		{
			for(xi=0;xi<16;xi++)
			{
				int pen = (state->m_gvram[count] >> xi) & 1;

				if(y < 475 && x*16+xi < 640) /* TODO: safety check */
					*BITMAP_ADDR16(bitmap, y, x*16+xi) = screen->machine().pens[pen];
			}

			count++;
		}
	}
	#endif

//	popmessage("%d %d %d",mc6845_h_display,mc6845_v_display,mc6845_tile_height);

	for(y=0;y<mc6845_v_display;y++)
	{
		for(x=0;x<mc6845_h_display;x++)
		{
			int tile = state->m_vram[x+y*mc6845_h_display] & 0xff;
			int color = (state->m_vram[x+y*mc6845_h_display] & 0x700) >> 8;
			int pen;

			for(yi=0;yi<19;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					pen = (gfx_rom[tile*8+(yi >> 1)] >> (7-xi) & 1) ? color : -1;

					if(yi & 0x10)
						pen = -1;

					//if(pen != -1)
						if(y*19 < 475 && x*8+xi < 640) /* TODO: safety check */
							*BITMAP_ADDR16(bitmap, y*19+yi, x*8+xi) = screen->machine().pens[pen];
				}
			}
		}
	}

	/* quick and dirty way to do the cursor */
	if(0)
	for(yi=0;yi<mc6845_tile_height;yi++)
	{
		for(xi=0;xi<8;xi++)
		{
			if((mc6845_cursor_y_start & 0x60) != 0x20 && mc6845_h_display)
			{
				x = mc6845_cursor_addr % mc6845_h_display;
				y = mc6845_cursor_addr / mc6845_h_display;
				*BITMAP_ADDR16(bitmap, y*mc6845_tile_height+yi, x*8+xi) = screen->machine().pens[7];
			}
		}
	}

	return 0;
}

static READ8_HANDLER( paso1600_pcg_r )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	return state->m_char_rom[offset];
}

static WRITE8_HANDLER( paso1600_pcg_w )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	state->m_char_rom[offset] = data;
	gfx_element_mark_dirty(space->machine().gfx[0], offset >> 3);
}

static WRITE8_HANDLER( paso1600_6845_address_w )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	state->m_crtc_index = data;
	state->m_mc6845->address_w(*space, offset, data);
}

static WRITE8_HANDLER( paso1600_6845_data_w )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	state->m_crtc_vreg[state->m_crtc_index] = data;
	state->m_mc6845->register_w(*space, offset, data);
}

static READ8_HANDLER( test_r )
{
	return 0;
}

static READ8_HANDLER( key_r )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	switch(offset)
	{
		case 3:
			if(state->m_keyb.portb == 1)
				return 0;

			return 0xff;
	}

	return 0xff;
}

static WRITE8_HANDLER( key_w )
{
	paso1600_state *state = space->machine().driver_data<paso1600_state>();

	switch(offset)
	{
		case 3: state->m_keyb.portb = data; break;
	}
}

static READ16_HANDLER( test_hi_r )
{
	return 0xffff;
}

static ADDRESS_MAP_START(paso1600_map, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0x7ffff) AM_RAM
	AM_RANGE(0xb0000,0xb0fff) AM_RAM AM_BASE_MEMBER(paso1600_state,m_vram) // tvram
	AM_RANGE(0xbfff0,0xbffff) AM_READWRITE8(paso1600_pcg_r,paso1600_pcg_w,0xffff)
	AM_RANGE(0xc0000,0xdffff) AM_RAM AM_BASE_MEMBER(paso1600_state,m_gvram)// gvram
	AM_RANGE(0xe0000,0xeffff) AM_ROM AM_REGION("kanji",0)// kanji rom, banked via port 0x93
	AM_RANGE(0xfe000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(paso1600_io, AS_IO, 16)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000,0x000f) AM_DEVREADWRITE8("8237dma", i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x0010,0x0011) AM_DEVREADWRITE8("pic8259", pic8259_r,pic8259_w, 0xffff) // i8259
	AM_RANGE(0x001a,0x001b) AM_READ(test_hi_r) // causes RAM error otherwise?
	AM_RANGE(0x0030,0x0033) AM_READWRITE8(key_r,key_w,0xffff) //UART keyboard?
	AM_RANGE(0x0048,0x0049) AM_READ(test_hi_r)
	AM_RANGE(0x0090,0x0091) AM_READWRITE8(test_r,paso1600_6845_address_w,0x00ff)
	AM_RANGE(0x0090,0x0091) AM_READWRITE8(test_r,paso1600_6845_data_w,0xff00)
//	AM_RANGE(0x00d8,0x00df) //fdc, unknown type
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( paso1600 )
INPUT_PORTS_END

static const gfx_layout paso1600_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static GFXDECODE_START( paso1600 )
	GFXDECODE_ENTRY( "pcg", 0x0000, paso1600_charlayout, 0, 1 )
GFXDECODE_END


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

static IRQ_CALLBACK(paso1600_irq_callback)
{
	return pic8259_acknowledge( device->machine().device( "pic8259" ) );
}

static WRITE_LINE_DEVICE_HANDLER( paso1600_set_int_line )
{
	//printf("%02x\n",interrupt);
	cputag_set_input_line(device->machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static const struct pic8259_interface paso1600_pic8259_config =
{
	DEVCB_LINE(paso1600_set_int_line),
	DEVCB_LINE_GND,
	DEVCB_NULL
};

static MACHINE_START(paso1600)
{
	paso1600_state *state = machine.driver_data<paso1600_state>();

	state->m_char_rom = machine.region("pcg")->base();
	state->m_mc6845 = machine.device<mc6845_device>("crtc");
	device_set_irq_callback(machine.device("maincpu"), paso1600_irq_callback);
}


static MACHINE_RESET(paso1600)
{
}

static READ8_HANDLER( pc_dma_read_byte )
{
//	paso1600_state *state = space->machine().driver_data<paso1600_state>();
	//offs_t page_offset = (((offs_t) state->m_dma_offset[0][state->m_dma_channel]) << 16)
	//	& 0xFF0000;

	return space->read_byte(0 + offset);
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
//	paso1600_state *state = space->machine().driver_data<paso1600_state>();
	//offs_t page_offset = (((offs_t) state->m_dma_offset[0][state->m_dma_channel]) << 16)
	//	& 0xFF0000;

	space->write_byte(0 + offset, data);
}

static I8237_INTERFACE( paso1600_dma8237_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


static MACHINE_CONFIG_START( paso1600, paso1600_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 16000000/2)
	MCFG_CPU_PROGRAM_MAP(paso1600_map)
	MCFG_CPU_IO_MAP(paso1600_io)

	MCFG_MACHINE_START(paso1600)
	MCFG_MACHINE_RESET(paso1600)

	MCFG_PIC8259_ADD( "pic8259", paso1600_pic8259_config )
	MCFG_I8237_ADD("8237dma", 16000000/4, paso1600_dma8237_interface)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(paso1600)

	MCFG_MC6845_ADD("crtc", H46505, 16000000/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

	MCFG_GFXDECODE(paso1600)
	MCFG_PALETTE_LENGTH(8)
//	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(paso1600)
MACHINE_CONFIG_END

ROM_START( paso1600 )
	ROM_REGION16_LE(0x2000,"ipl", 0)
	ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(cee4ebb7) SHA1(c23b30f8dc51f96c1c00e28aab61e77b50d261f0))

	ROM_REGION(0x2000,"pcg", ROMREGION_ERASE00)

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, BAD_DUMP CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412)) //stolen from pasopia7

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, NO_DUMP)

ROM_END


/*    YEAR  NAME        PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP ( 198?,paso1600,	0,	    0,       paso1600,	paso1600, 0,	"Toshiba",  "Pasopia 1600" , GAME_NOT_WORKING|GAME_NO_SOUND)
