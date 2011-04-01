/***************************************************************************

    CPS Changer

***************************************************************************/

#include "emu.h"
#include "includes/cpschngr.h"

/********************************************************************

            Configuration table:

********************************************************************/

/* Game specific data */

#define GFXTYPE_SPRITES   (1<<0)
#define GFXTYPE_SCROLL1   (1<<1)
#define GFXTYPE_SCROLL2   (1<<2)
#define GFXTYPE_SCROLL3   (1<<3)
#define GFXTYPE_STARS     (1<<4)


struct gfx_range
{
	// start and end are as passed by the game (shift adjusted to be all
	// in the same scale a 8x8 tiles): they don't necessarily match the
	// position in ROM.
	int type;
	int start;
	int end;
	int bank;
};

struct CPS1config
{
	const char *name;             /* game driver name */

	/* Some games interrogate a couple of registers on bootup. */
	/* These are CPS1 board B self test checks. They wander from game to */
	/* game. */
	int cpsb_addr;        /* CPS board B test register address */
	int cpsb_value;       /* CPS board B test register expected value */

	/* some games use as a protection check the ability to do 16-bit multiplies */
	/* with a 32-bit result, by writing the factors to two ports and reading the */
	/* result from two other ports. */
	/* It looks like this feature was introduced with 3wonders (CPSB ID = 08xx) */
	int mult_factor1;
	int mult_factor2;
	int mult_result_lo;
	int mult_result_hi;

	/* unknown registers which might be related to the multiply protection */
	int unknown1;
	int unknown2;
	int unknown3;

	int layer_control;
	int priority[4];
	int palette_control;

	/* ideally, the layer enable masks should consist of only one bit, */
	/* but in many cases it is unknown which bit is which. */
	int layer_enable_mask[5];

	/* these depend on the B-board model and PAL */
	int bank_sizes[4];
	const struct gfx_range *bank_mapper;

	/* some C-boards have additional I/O for extra buttons/extra players */
	int in2_addr;
	int in3_addr;
	int out2_addr;

	int bootleg_kludge;
};

#define __not_applicable__	-1,-1,-1,-1,-1,-1,-1

/*                     CPSB ID    multiply protection      unknown      ctrl     priority masks   palctrl    layer enable masks  */
#define CPS_B_21_DEF 0x32,  -1,   0x00,0x02,0x04,0x06, 0x08, -1,  -1,   0x26,{0x28,0x2a,0x2c,0x2e},0x30, {0x02,0x04,0x08,0x30,0x30}	// pang3 sets layer enable to 0x26 on startup

#define mapper_sfzch	{ 0x20000, 0, 0, 0 }, mapper_sfzch_table
static const struct gfx_range mapper_sfzch_table[] =
{
	/* type                                                                  start    end      bank */
	{ GFXTYPE_SPRITES | GFXTYPE_SCROLL1 | GFXTYPE_SCROLL2 | GFXTYPE_SCROLL3, 0x00000, 0x1ffff, 0 },
	{ 0 }
};

static const struct CPS1config cps1_config_table[]=
{
	/* name       CPSB         gfx mapper   in2  in3  out2   kludge */
	{"sfach",    CPS_B_21_DEF, mapper_sfzch },
	{"sfzbch",   CPS_B_21_DEF, mapper_sfzch },
	{"sfzch",    CPS_B_21_DEF, mapper_sfzch },
	{"wofch",    CPS_B_21_DEF, mapper_sfzch },
	{0}		/* End of table */
};


/* Offset of each palette entry */
#define cps1_palette_entries (32*6)  /* Number colour schemes in palette */

enum
{
	cps1_scroll_size =0x4000,	/* scroll1, scroll2, scroll3 */
	cps1_obj_size    =0x0800,
	cps1_other_size  =0x0800,
	cps1_palette_align=0x0400,	/* minimum alignment is a single palette page (512 colors). Verified on pcb. */
	cps1_palette_size=cps1_palette_entries*32, /* Size of palette RAM */
	/* first 0x4000 of gfx ROM are used, but 0x0000-0x1fff is == 0x2000-0x3fff */
	cps1_stars_rom_size = 0x2000,

};


/* CPS-A registers */
#define CPS1_OBJ_BASE           (0x00/2)    /* Base address of objects */
#define CPS1_SCROLL1_BASE       (0x02/2)    /* Base address of scroll 1 */
#define CPS1_SCROLL2_BASE       (0x04/2)    /* Base address of scroll 2 */
#define CPS1_SCROLL3_BASE       (0x06/2)    /* Base address of scroll 3 */
#define CPS1_OTHER_BASE         (0x08/2)    /* Base address of other video */
#define CPS1_PALETTE_BASE       (0x0a/2)    /* Base address of palette */
#define CPS1_SCROLL1_SCROLLX    (0x0c/2)    /* Scroll 1 X */
#define CPS1_SCROLL1_SCROLLY    (0x0e/2)    /* Scroll 1 Y */
#define CPS1_SCROLL2_SCROLLX    (0x10/2)    /* Scroll 2 X */
#define CPS1_SCROLL2_SCROLLY    (0x12/2)    /* Scroll 2 Y */
#define CPS1_SCROLL3_SCROLLX    (0x14/2)    /* Scroll 3 X */
#define CPS1_SCROLL3_SCROLLY    (0x16/2)    /* Scroll 3 Y */
#define CPS1_STARS1_SCROLLX     (0x18/2)    /* Stars 1 X */
#define CPS1_STARS1_SCROLLY     (0x1a/2)    /* Stars 1 Y */
#define CPS1_STARS2_SCROLLX     (0x1c/2)    /* Stars 2 X */
#define CPS1_STARS2_SCROLLY     (0x1e/2)    /* Stars 2 Y */
#define CPS1_ROWSCROLL_OFFS     (0x20/2)    /* base of row scroll offsets in other RAM */
#define CPS1_VIDEOCONTROL       (0x22/2)    /* flip screen, rowscroll enable */


static void cps1_build_palette(running_machine &machine, const UINT16* const palette_base);


static MACHINE_RESET( cps )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	const char *gamename = machine.system().name;
	const struct CPS1config *pCFG=&cps1_config_table[0];

	while(pCFG->name)
	{
		if (strcmp(pCFG->name, gamename) == 0)
		{
			break;
		}
		pCFG++;
	}
	state->m_cps1_game_config=pCFG;
}


INLINE UINT16 *cps1_base(cpschngr_state *state, int offset, int boundary)
{
	int base = state->m_cps1_cps_a_regs[offset]*256;
	base &= ~(boundary-1);
	return &state->m_cps1_gfxram[(base&0x3ffff)/2];
}



WRITE16_HANDLER( cps1_cps_a_w )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	data = COMBINE_DATA(&state->m_cps1_cps_a_regs[offset]);

	if (offset == CPS1_PALETTE_BASE)
		cps1_build_palette(space->machine(), cps1_base(state, CPS1_PALETTE_BASE, cps1_palette_align));
}


READ16_HANDLER( cps1_cps_b_r )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	/* Some games interrogate a couple of registers on bootup. */
	/* These are CPS1 board B self test checks. They wander from game to */
	/* game. */
	if (offset == state->m_cps1_game_config->cpsb_addr/2)
		return state->m_cps1_game_config->cpsb_value;

	/* some games use as a protection check the ability to do 16-bit multiplications */
	/* with a 32-bit result, by writing the factors to two ports and reading the */
	/* result from two other ports. */
	if (offset == state->m_cps1_game_config->mult_result_lo/2)
		return (state->m_cps1_cps_b_regs[state->m_cps1_game_config->mult_factor1/2] *
				state->m_cps1_cps_b_regs[state->m_cps1_game_config->mult_factor2/2]) & 0xffff;
	if (offset == state->m_cps1_game_config->mult_result_hi/2)
		return (state->m_cps1_cps_b_regs[state->m_cps1_game_config->mult_factor1/2] *
				state->m_cps1_cps_b_regs[state->m_cps1_game_config->mult_factor2/2]) >> 16;

	if (offset == state->m_cps1_game_config->in2_addr/2)	/* Extra input ports (on C-board) */
		return input_port_read(space->machine(), "IN2");

	if (offset == state->m_cps1_game_config->in3_addr/2)	/* Player 4 controls (on C-board) ("Captain Commando") */
		return input_port_read(space->machine(), "IN3");

	return 0xffff;
}


WRITE16_HANDLER( cps1_cps_b_w )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	data = COMBINE_DATA(&state->m_cps1_cps_b_regs[offset]);

	// additional outputs on C-board
	if (offset == state->m_cps1_game_config->out2_addr/2)
	{
		if (ACCESSING_BITS_0_7)
		{
			if (state->m_cps1_game_config->cpsb_value == 0x0402)	// Mercs (CN2 connector)
			{
				coin_lockout_w(space->machine(),2,~data & 0x01);
				set_led_status(space->machine(),0,data & 0x02);
				set_led_status(space->machine(),1,data & 0x04);
				set_led_status(space->machine(),2,data & 0x08);
			}
			else	// kod, captcomm, knights
			{
				coin_lockout_w(space->machine(),2,~data & 0x02);
				coin_lockout_w(space->machine(),3,~data & 0x08);
			}
		}
	}
}



static void cps1_gfx_decode(running_machine &machine)
{
	int size=machine.region("gfx")->bytes();
	int i,j,gfxsize;
	UINT8 *cps1_gfx = machine.region("gfx")->base();


	gfxsize=size/4;

	for (i = 0;i < gfxsize;i++)
	{
		UINT32 src = cps1_gfx[4*i] + (cps1_gfx[4*i+1]<<8) + (cps1_gfx[4*i+2]<<16) + (cps1_gfx[4*i+3]<<24);
		UINT32 dwval = 0;

		for (j = 0;j < 8;j++)
		{
			int n = 0;
			UINT32 mask = (0x80808080 >> j) & src;

			if (mask & 0x000000ff) n |= 1;
			if (mask & 0x0000ff00) n |= 2;
			if (mask & 0x00ff0000) n |= 4;
			if (mask & 0xff000000) n |= 8;

			dwval |= n << (j * 4);
		}
		cps1_gfx[4*i  ] = dwval>>0;
		cps1_gfx[4*i+1] = dwval>>8;
		cps1_gfx[4*i+2] = dwval>>16;
		cps1_gfx[4*i+3] = dwval>>24;
	}
}

#ifdef UNUSED_FUNCTION
static void unshuffle(UINT64 *buf,int len)
{
	int i;
	UINT64 t;

	if (len == 2) return;

	assert(len % 4 == 0);   /* must not happen */

	len /= 2;

	unshuffle(buf,len);
	unshuffle(buf + len,len);

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}
}
#endif


DRIVER_INIT( cps1 )
{
	cps1_gfx_decode(machine);
}


static void cps1_get_video_base(cpschngr_state *state)
{
	int layercontrol, videocontrol, scroll1xoff, scroll2xoff, scroll3xoff;

	/* Re-calculate the VIDEO RAM base */
	if (state->m_cps1_scroll1 != cps1_base(state, CPS1_SCROLL1_BASE, cps1_scroll_size))
	{
		state->m_cps1_scroll1 = cps1_base(state, CPS1_SCROLL1_BASE, cps1_scroll_size);
		tilemap_mark_all_tiles_dirty(state->m_cps1_bg_tilemap[0]);
	}
	if (state->m_cps1_scroll2 != cps1_base(state, CPS1_SCROLL2_BASE, cps1_scroll_size))
	{
		state->m_cps1_scroll2 = cps1_base(state, CPS1_SCROLL2_BASE, cps1_scroll_size);
		tilemap_mark_all_tiles_dirty(state->m_cps1_bg_tilemap[1]);
	}
	if (state->m_cps1_scroll3 != cps1_base(state, CPS1_SCROLL3_BASE, cps1_scroll_size))
	{
		state->m_cps1_scroll3 = cps1_base(state, CPS1_SCROLL3_BASE, cps1_scroll_size);
		tilemap_mark_all_tiles_dirty(state->m_cps1_bg_tilemap[2]);
	}

	/* Some of the sf2 hacks use only sprite port 0x9100 and the scroll layers are offset */
	if (state->m_cps1_game_config->bootleg_kludge == 1)
	{
		state->m_cps1_cps_a_regs[CPS1_OBJ_BASE] = 0x9100;
		state->m_cps1_obj = cps1_base(state, CPS1_OBJ_BASE, cps1_obj_size);
		scroll1xoff = -0x0c;
		scroll2xoff = -0x0e;
		scroll3xoff = -0x10;
	}
	else
	{
		state->m_cps1_obj = cps1_base(state, CPS1_OBJ_BASE, cps1_obj_size);
		scroll1xoff = 0;
		scroll2xoff = 0;
		scroll3xoff = 0;
	}
	state->m_cps1_other = cps1_base(state, CPS1_OTHER_BASE, cps1_other_size);

	/* Get scroll values */
	state->m_cps1_scroll1x = state->m_cps1_cps_a_regs[CPS1_SCROLL1_SCROLLX] + scroll1xoff;
	state->m_cps1_scroll1y = state->m_cps1_cps_a_regs[CPS1_SCROLL1_SCROLLY];
	state->m_cps1_scroll2x = state->m_cps1_cps_a_regs[CPS1_SCROLL2_SCROLLX] + scroll2xoff;
	state->m_cps1_scroll2y = state->m_cps1_cps_a_regs[CPS1_SCROLL2_SCROLLY];
	state->m_cps1_scroll3x = state->m_cps1_cps_a_regs[CPS1_SCROLL3_SCROLLX] + scroll3xoff;
	state->m_cps1_scroll3y = state->m_cps1_cps_a_regs[CPS1_SCROLL3_SCROLLY];
	state->m_stars1x = state->m_cps1_cps_a_regs[CPS1_STARS1_SCROLLX];
	state->m_stars1y = state->m_cps1_cps_a_regs[CPS1_STARS1_SCROLLY];
	state->m_stars2x = state->m_cps1_cps_a_regs[CPS1_STARS2_SCROLLX];
	state->m_stars2y = state->m_cps1_cps_a_regs[CPS1_STARS2_SCROLLY];

	/* Get layer enable bits */
	layercontrol = state->m_cps1_cps_b_regs[state->m_cps1_game_config->layer_control/2];
	videocontrol = state->m_cps1_cps_a_regs[CPS1_VIDEOCONTROL];
	tilemap_set_enable(state->m_cps1_bg_tilemap[0],layercontrol & state->m_cps1_game_config->layer_enable_mask[0]);
	tilemap_set_enable(state->m_cps1_bg_tilemap[1],(layercontrol & state->m_cps1_game_config->layer_enable_mask[1]) && (videocontrol & 4));
	tilemap_set_enable(state->m_cps1_bg_tilemap[2],(layercontrol & state->m_cps1_game_config->layer_enable_mask[2]) && (videocontrol & 8));
	state->m_cps1_stars_enabled[0] = layercontrol & state->m_cps1_game_config->layer_enable_mask[3];
	state->m_cps1_stars_enabled[1] = layercontrol & state->m_cps1_game_config->layer_enable_mask[4];
}


WRITE16_HANDLER( cps1_gfxram_w )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	int page = (offset >> 7) & 0x3c0;
	COMBINE_DATA(&state->m_cps1_gfxram[offset]);

	if (page == (state->m_cps1_cps_a_regs[CPS1_SCROLL1_BASE] & 0x3c0))
		tilemap_mark_tile_dirty(state->m_cps1_bg_tilemap[0],offset/2 & 0x0fff);
	if (page == (state->m_cps1_cps_a_regs[CPS1_SCROLL2_BASE] & 0x3c0))
		tilemap_mark_tile_dirty(state->m_cps1_bg_tilemap[1],offset/2 & 0x0fff);
	if (page == (state->m_cps1_cps_a_regs[CPS1_SCROLL3_BASE] & 0x3c0))
		tilemap_mark_tile_dirty(state->m_cps1_bg_tilemap[2],offset/2 & 0x0fff);
}



static int gfxrom_bank_mapper(running_machine &machine, int type, int code)
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	const struct gfx_range *range = state->m_cps1_game_config->bank_mapper;
	int shift = 0;

	assert(range);

	switch (type)
	{
		case GFXTYPE_SPRITES: shift = 1; break;
		case GFXTYPE_SCROLL1: shift = 0; break;
		case GFXTYPE_SCROLL2: shift = 1; break;
		case GFXTYPE_SCROLL3: shift = 3; break;
	}

	code <<= shift;

	while (range->type)
	{
		if (code >= range->start && code <= range->end)
		{
			if (range->type & type)
			{
				int base = 0;
				int i;

				for (i = 0; i < range->bank; ++i)
					base += state->m_cps1_game_config->bank_sizes[i];

				return (base + (code & (state->m_cps1_game_config->bank_sizes[range->bank] - 1))) >> shift;
			}
		}

		++range;
	}

	return -1;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static TILEMAP_MAPPER( tilemap0_scan )
{
	/* logical (col,row) -> memory offset */
	return (row & 0x1f) + ((col & 0x3f) << 5) + ((row & 0x20) << 6);
}

static TILEMAP_MAPPER( tilemap1_scan )
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0x3f) << 4) + ((row & 0x30) << 6);
}

static TILEMAP_MAPPER( tilemap2_scan )
{
	/* logical (col,row) -> memory offset */
	return (row & 0x07) + ((col & 0x3f) << 3) + ((row & 0x38) << 6);
}


static TILE_GET_INFO( get_tile0_info )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	int code = state->m_cps1_scroll1[2*tile_index];
	int attr = state->m_cps1_scroll1[2*tile_index+1];
	int gfxset;

	code = gfxrom_bank_mapper(machine, GFXTYPE_SCROLL1, code);

	/* allows us to reproduce a problem seen with a ffight board where USA and Japanese
         roms have been mixed to be reproduced (ffightua) -- it looks like each column
         should alternate between the left and right side of the 16x16 tiles */
	gfxset = (tile_index & 0x20) >> 5;

	SET_TILE_INFO(
			gfxset,
			code,
			(attr & 0x1f) + 0x20,
			TILE_FLIPYX((attr & 0x60) >> 5));
	tileinfo->group = (attr & 0x0180) >> 7;

	// for out of range tiles, switch to fully transparent data
	// (but still call SET_TILE_INFO, otherwise problems might occur on boot e.g. unsquad)
	if (code == -1)
		tileinfo->pen_data = state->m_empty_tile8x8;
}

static TILE_GET_INFO( get_tile1_info )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	int code = state->m_cps1_scroll2[2*tile_index];
	int attr = state->m_cps1_scroll2[2*tile_index+1];

	code = gfxrom_bank_mapper(machine, GFXTYPE_SCROLL2, code);

	SET_TILE_INFO(
			2,
			code,
			(attr & 0x1f) + 0x40,
			TILE_FLIPYX((attr & 0x60) >> 5));
	tileinfo->group = (attr & 0x0180) >> 7;

	// for out of range tiles, switch to fully transparent data
	if (code == -1)
		tileinfo->pen_data = state->m_empty_tile;
}

static TILE_GET_INFO( get_tile2_info )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	int code = state->m_cps1_scroll3[2*tile_index] & 0x3fff;
	int attr = state->m_cps1_scroll3[2*tile_index+1];

	code = gfxrom_bank_mapper(machine, GFXTYPE_SCROLL3, code);

	SET_TILE_INFO(
			3,
			code,
			(attr & 0x1f) + 0x60,
			TILE_FLIPYX((attr & 0x60) >> 5));
	tileinfo->group = (attr & 0x0180) >> 7;

	// for out of range tiles, switch to fully transparent data
	// (but still call SET_TILE_INFO, otherwise problems might occur on boot e.g. unsquad)
	if (code == -1)
		tileinfo->pen_data = state->m_empty_tile;
}



static void cps1_update_transmasks(cpschngr_state *state)
{
	int i;

	for (i = 0;i < 4;i++)
	{
		int mask;

		/* Get transparency registers */
		if (state->m_cps1_game_config->priority[i] != -1)
			mask = state->m_cps1_cps_b_regs[state->m_cps1_game_config->priority[i]/2] ^ 0xffff;
		else mask = 0xffff;	/* completely transparent if priority masks not defined (qad) */

		tilemap_set_transmask(state->m_cps1_bg_tilemap[0],i,mask,0x8000);
		tilemap_set_transmask(state->m_cps1_bg_tilemap[1],i,mask,0x8000);
		tilemap_set_transmask(state->m_cps1_bg_tilemap[2],i,mask,0x8000);
	}
}

VIDEO_START( cps1 )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	int i;

	MACHINE_RESET_CALL(cps);

	state->m_cps1_bg_tilemap[0] = tilemap_create(machine, get_tile0_info,tilemap0_scan, 8, 8,64,64);
	state->m_cps1_bg_tilemap[1] = tilemap_create(machine, get_tile1_info,tilemap1_scan,16,16,64,64);
	state->m_cps1_bg_tilemap[2] = tilemap_create(machine, get_tile2_info,tilemap2_scan,32,32,64,64);

	/* front masks will change at runtime to handle sprite occluding */
	cps1_update_transmasks(state);

	memset(state->m_empty_tile8x8,0x0f,sizeof(state->m_empty_tile8x8));
	memset(state->m_empty_tile,0xff,sizeof(state->m_empty_tile));	// 16x16 and 32x32 use packed graphics, 8x8 does not

	for (i = 0;i < cps1_palette_entries*16;i++)
	{
		palette_set_color(machine,i,MAKE_RGB(0,0,0));
	}

	state->m_cps1_buffered_obj = auto_alloc_array_clear(machine, UINT16, cps1_obj_size / sizeof(UINT16));

	memset(state->m_cps1_gfxram, 0, state->m_cps1_gfxram_size);   /* Clear GFX RAM */
	memset(state->m_cps1_cps_a_regs, 0, 0x40);   /* Clear CPS-A registers */
	memset(state->m_cps1_cps_b_regs, 0, 0x40);   /* Clear CPS-B registers */

	/* Put in some defaults */
	state->m_cps1_cps_a_regs[CPS1_OBJ_BASE]     = 0x9200;
	state->m_cps1_cps_a_regs[CPS1_SCROLL1_BASE] = 0x9000;
	state->m_cps1_cps_a_regs[CPS1_SCROLL2_BASE] = 0x9040;
	state->m_cps1_cps_a_regs[CPS1_SCROLL3_BASE] = 0x9080;
	state->m_cps1_cps_a_regs[CPS1_OTHER_BASE]   = 0x9100;

	assert_always(state->m_cps1_game_config, "state->m_cps1_game_config hasn't been set up yet");

	/* Set up old base */
	cps1_get_video_base(state);   /* Calculate base pointers */
	cps1_get_video_base(state);   /* Calculate old base pointers */
}



/***************************************************************************

  Build palette from palette RAM

  12 bit RGB with a 4 bit brightness value.

***************************************************************************/

static void cps1_build_palette(running_machine &machine, const UINT16* const palette_base)
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	int offset, page;
	const UINT16 *palette_ram = palette_base;

	int ctrl = state->m_cps1_cps_b_regs[state->m_cps1_game_config->palette_control/2];

	for (page = 0; page < 6; ++page)
	{
		if (BIT(ctrl,page))
		{
			for (offset = 0; offset < 0x200; ++offset)
			{
				int palette = *(palette_ram++);
				int r, g, b, bright;

				// from my understanding of the schematics, when the 'brightness'
				// component is set to 0 it should reduce brightness to 1/3

				bright = 0x0f + ((palette>>12)<<1);

				r = ((palette>>8)&0x0f) * 0x11 * bright / 0x2d;
				g = ((palette>>4)&0x0f) * 0x11 * bright / 0x2d;
				b = ((palette>>0)&0x0f) * 0x11 * bright / 0x2d;

				palette_set_color (machine, 0x200*page + offset, MAKE_RGB(r, g, b));
			}
		}
		else
		{
			// skip page in gfxram, but only if we have already copied at least one page
			if (palette_ram != palette_base)
				palette_ram += 0x200;
		}
	}
}

static void cps1_find_last_sprite(cpschngr_state *state)    /* Find the offset of last sprite */
{
	int offset=0;
	/* Locate the end of table marker */
	while (offset < cps1_obj_size/2)
	{
        int colour=state->m_cps1_buffered_obj[offset+3];
		if ((colour & 0xff00) == 0xff00)
		{
			/* Marker found. This is the last sprite. */
			state->m_cps1_last_sprite_offset=offset-4;
			return;
		}
        offset+=4;
	}
	/* Sprites must use full sprite RAM */
	state->m_cps1_last_sprite_offset=cps1_obj_size/2-4;
}


static void cps1_render_sprites(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
#define DRAWSPRITE(CODE,COLOR,FLIPX,FLIPY,SX,SY)					\
{																	\
	if (flip_screen_get(machine))											\
		pdrawgfx_transpen(bitmap,\
				cliprect,machine.gfx[2],							\
				CODE,												\
				COLOR,												\
				!(FLIPX),!(FLIPY),									\
				511-16-(SX),255-16-(SY),							machine.priority_bitmap,0x02,15);					\
	else															\
		pdrawgfx_transpen(bitmap,\
				cliprect,machine.gfx[2],							\
				CODE,												\
				COLOR,												\
				FLIPX,FLIPY,										\
				SX,SY,												machine.priority_bitmap,0x02,15);					\
}


	int i, baseadd;
	UINT16 *base=state->m_cps1_buffered_obj;

	/* some sf2 hacks draw the sprites in reverse order */
	if (state->m_cps1_game_config->bootleg_kludge == 1)
	{
		base += state->m_cps1_last_sprite_offset;
		baseadd = -4;
	}
	else
	{
		baseadd = 4;
	}

	for (i=state->m_cps1_last_sprite_offset; i>=0; i-=4)
	{
		int x=*(base+0);
		int y=*(base+1);
		int code  =*(base+2);
		int colour=*(base+3);
		int col=colour&0x1f;

		code = gfxrom_bank_mapper(machine, GFXTYPE_SPRITES, code);

		if (code != -1)
		{
			if (colour & 0xff00 )
			{
				/* handle blocked sprites */
				int nx=(colour & 0x0f00) >> 8;
				int ny=(colour & 0xf000) >> 12;
				int nxs,nys,sx,sy;
				nx++;
				ny++;

				if (colour & 0x40)
				{
					/* Y flip */
					if (colour &0x20)
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x1ff;
								sy = (y+nys*16) & 0x1ff;

								DRAWSPRITE(
										(code & ~0xf) + ((code + (nx-1) - nxs) & 0xf) + 0x10*(ny-1-nys),
										(col&0x1f),
										1,1,
										sx,sy);
							}
						}
					}
					else
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x1ff;
								sy = (y+nys*16) & 0x1ff;

								DRAWSPRITE(
										(code & ~0xf) + ((code + nxs) & 0xf) + 0x10*(ny-1-nys),
										(col&0x1f),
										0,1,
										sx,sy);
							}
						}
					}
				}
				else
				{
					if (colour &0x20)
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x1ff;
								sy = (y+nys*16) & 0x1ff;

								DRAWSPRITE(
										(code & ~0xf) + ((code + (nx-1) - nxs) & 0xf) + 0x10*nys,
										(col&0x1f),
										1,0,
										sx,sy);
							}
						}
					}
					else
					{
						for (nys=0; nys<ny; nys++)
						{
							for (nxs=0; nxs<nx; nxs++)
							{
								sx = (x+nxs*16) & 0x1ff;
								sy = (y+nys*16) & 0x1ff;

								DRAWSPRITE(
										(code & ~0xf) + ((code + nxs) & 0xf) + 0x10*nys,	// fix 00406: qadj: When playing as the ninja, there is one broekn frame in his animation loop when walking.
										(col&0x1f),
										0,0,
										sx,sy);
							}
						}
					}
				}
			}
			else
			{
				/* Simple case... 1 sprite */
						DRAWSPRITE(
						code,
						(col&0x1f),
						colour&0x20,colour&0x40,
						x & 0x1ff,y & 0x1ff);
			}
		}
		base += baseadd;
	}
#undef DRAWSPRITE
}


static void cps1_render_stars(screen_device *screen, bitmap_t *bitmap,const rectangle *cliprect)
{
	cpschngr_state *state = screen->machine().driver_data<cpschngr_state>();
	int offs;
	UINT8 *stars_rom = screen->machine().region("stars")->base();

	if (!stars_rom && (state->m_cps1_stars_enabled[0] || state->m_cps1_stars_enabled[1]))
	{
		return;
	}

	if (state->m_cps1_stars_enabled[0])
	{
		for (offs = 0;offs < cps1_stars_rom_size/2;offs++)
		{
			int col = stars_rom[8*offs+4];
			if (col != 0x0f)
			{
				int sx = (offs / 256) * 32;
				int sy = (offs % 256);
				sx = (sx - state->m_stars2x + (col & 0x1f)) & 0x1ff;
				sy = (sy - state->m_stars2y) & 0xff;
				if (flip_screen_get(screen->machine()))
				{
					sx = 511 - sx;
					sy = 255 - sy;
				}

				col = ((col & 0xe0) >> 1) + (screen->frame_number()/16 & 0x0f);

				if (sx >= cliprect->min_x && sx <= cliprect->max_x &&
					sy >= cliprect->min_y && sy <= cliprect->max_y)
					*BITMAP_ADDR16(bitmap, sy, sx) = 0xa00 + col;
			}
		}
	}

	if (state->m_cps1_stars_enabled[1])
	{
		for (offs = 0;offs < cps1_stars_rom_size/2;offs++)
		{
			int col = stars_rom[8*offs];
			if (col != 0x0f)
			{
				int sx = (offs / 256) * 32;
				int sy = (offs % 256);
				sx = (sx - state->m_stars1x + (col & 0x1f)) & 0x1ff;
				sy = (sy - state->m_stars1y) & 0xff;
				if (flip_screen_get(screen->machine()))
				{
					sx = 511 - sx;
					sy = 255 - sy;
				}

				col = ((col & 0xe0) >> 1) + (screen->frame_number()/16 & 0x0f);

				if (sx >= cliprect->min_x && sx <= cliprect->max_x &&
					sy >= cliprect->min_y && sy <= cliprect->max_y)
					*BITMAP_ADDR16(bitmap, sy, sx) = 0x800 + col;
			}
		}
	}
}


static void cps1_render_layer(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, int layer, int primask)
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	switch (layer)
	{
		case 0:
			cps1_render_sprites(machine,bitmap,cliprect);
			break;
		case 1:
		case 2:
		case 3:
			tilemap_draw(bitmap,cliprect,state->m_cps1_bg_tilemap[layer-1],TILEMAP_DRAW_LAYER1,primask);
			break;
	}
}

static void cps1_render_high_layer(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, int layer)
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	switch (layer)
	{
		case 0:
			/* there are no high priority sprites */
			break;
		case 1:
		case 2:
		case 3:
			tilemap_draw(NULL,cliprect,state->m_cps1_bg_tilemap[layer-1],TILEMAP_DRAW_LAYER0,1);
			break;
	}
}


/***************************************************************************

    Refresh screen

***************************************************************************/

SCREEN_UPDATE( cps1 )
{
	cpschngr_state *state = screen->machine().driver_data<cpschngr_state>();
	int layercontrol,l0,l1,l2,l3;
	int videocontrol = state->m_cps1_cps_a_regs[CPS1_VIDEOCONTROL];


	flip_screen_set(screen->machine(), videocontrol & 0x8000);

	layercontrol = state->m_cps1_cps_b_regs[state->m_cps1_game_config->layer_control/2];

	/* Get video memory base registers */
	cps1_get_video_base(state);

	/* Find the offset of the last sprite in the sprite table */
	cps1_find_last_sprite(state);
	cps1_update_transmasks(state);

	tilemap_set_scrollx(state->m_cps1_bg_tilemap[0],0,state->m_cps1_scroll1x);
	tilemap_set_scrolly(state->m_cps1_bg_tilemap[0],0,state->m_cps1_scroll1y);
	if (videocontrol & 0x01)	/* linescroll enable */
	{
		int scrly=-state->m_cps1_scroll2y;
		int i;
		int otheroffs;

		tilemap_set_scroll_rows(state->m_cps1_bg_tilemap[1],1024);

		otheroffs = state->m_cps1_cps_a_regs[CPS1_ROWSCROLL_OFFS];

		for (i = 0;i < 256;i++)
			tilemap_set_scrollx(state->m_cps1_bg_tilemap[1],(i - scrly) & 0x3ff,state->m_cps1_scroll2x + state->m_cps1_other[(i + otheroffs) & 0x3ff]);
	}
	else
	{
		tilemap_set_scroll_rows(state->m_cps1_bg_tilemap[1],1);
		tilemap_set_scrollx(state->m_cps1_bg_tilemap[1],0,state->m_cps1_scroll2x);
	}
	tilemap_set_scrolly(state->m_cps1_bg_tilemap[1],0,state->m_cps1_scroll2y);
	tilemap_set_scrollx(state->m_cps1_bg_tilemap[2],0,state->m_cps1_scroll3x);
	tilemap_set_scrolly(state->m_cps1_bg_tilemap[2],0,state->m_cps1_scroll3y);


	/* Blank screen */
	bitmap_fill(bitmap,cliprect,0xbff);

	cps1_render_stars(screen, bitmap,cliprect);

	/* Draw layers (0 = sprites, 1-3 = tilemaps) */
	l0 = (layercontrol >> 0x06) & 03;
	l1 = (layercontrol >> 0x08) & 03;
	l2 = (layercontrol >> 0x0a) & 03;
	l3 = (layercontrol >> 0x0c) & 03;
	bitmap_fill(screen->machine().priority_bitmap,cliprect,0);

	cps1_render_layer(screen->machine(),bitmap,cliprect,l0,0);
	if (l1 == 0) cps1_render_high_layer(screen->machine(), bitmap, cliprect, l0); /* prepare mask for sprites */
	cps1_render_layer(screen->machine(),bitmap,cliprect,l1,0);
	if (l2 == 0) cps1_render_high_layer(screen->machine(), bitmap, cliprect, l1); /* prepare mask for sprites */
	cps1_render_layer(screen->machine(),bitmap,cliprect,l2,0);
	if (l3 == 0) cps1_render_high_layer(screen->machine(), bitmap, cliprect, l2); /* prepare mask for sprites */
	cps1_render_layer(screen->machine(),bitmap,cliprect,l3,0);

	return 0;
}

SCREEN_EOF( cps1 )
{
	cpschngr_state *state = machine.driver_data<cpschngr_state>();
	/* Get video memory base registers */
	cps1_get_video_base(state);

	/* CPS1 sprites have to be delayed one frame */
	memcpy(state->m_cps1_buffered_obj, state->m_cps1_obj, cps1_obj_size);
}
