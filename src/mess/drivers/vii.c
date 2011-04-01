/******************************************************************************


    Vii / The Batman
    -------------------

    MESS driver by Harmony
    Based largely off of Unununium, by Segher


*******************************************************************************

    Short Description:

        Systems run on the SPG243 SoC

    Status:

        Mostly working

    To-Do:

        Audio (SPG243)
        Motion controls (Vii)

    Known u'nSP-Based Systems:

        ND - SPG243 - Some form of Leapfrog "edutainment" system
        ND - SPG243 - Star Wars: Clone Wars
        ND - SPG243 - Toy Story
        ND - SPG243 - Animal Art Studio
        ND - SPG243 - Finding Nemo
         D - SPG243 - The Batman
         D - SPG243 - Wall-E
         D - SPG243 - Chintendo / KenSingTon / Siatronics / Jungle Soft Vii
 Partial D - SPG200 - V-Tech V-Smile
        ND - Likely many more

*******************************************************************************/

#include "emu.h"
#include "cpu/unsp/unsp.h"
#include "imagedev/cartslot.h"
#include "machine/i2cmem.h"

#define PAGE_ENABLE_MASK		0x0008

#define PAGE_DEPTH_FLAG_MASK	0x3000
#define PAGE_DEPTH_FLAG_SHIFT	12
#define PAGE_TILE_HEIGHT_MASK	0x00c0
#define PAGE_TILE_HEIGHT_SHIFT	6
#define PAGE_TILE_WIDTH_MASK	0x0030
#define PAGE_TILE_WIDTH_SHIFT	4
#define TILE_X_FLIP				0x0004
#define TILE_Y_FLIP				0x0008

class vii_state : public driver_device
{
public:
	vii_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *m_ram;
	UINT16 *m_cart;
	UINT16 *m_rowscroll;
	UINT16 *m_palette;
	UINT16 *m_spriteram;

	UINT32 m_current_bank;

	UINT16 m_video_regs[0x100];
	UINT32 m_centered_coordinates;

	struct
	{
		UINT8 r, g, b;
	}
	m_screen[320*240];

	UINT16 m_io_regs[0x200];
	UINT16 m_uart_rx_count;
	UINT8 m_controller_input[8];
	UINT32 m_spg243_mode;
};

enum
{
	SPG243_VII = 0,
	SPG243_BATMAN,
	SPG243_VSMILE,

	SPG243_MODEL_COUNT,
};


#define VII_CTLR_IRQ_ENABLE		state->m_io_regs[0x21]
#define VII_VIDEO_IRQ_ENABLE	state->m_video_regs[0x62]
#define VII_VIDEO_IRQ_STATUS	state->m_video_regs[0x63]


#define VERBOSE_LEVEL	(3)

#define ENABLE_VERBOSE_LOG (1)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine &machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%04x: %s", cpu_get_pc(machine.device("maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

/*************************
*     Video Hardware     *
*************************/

static VIDEO_START( vii )
{
}

INLINE UINT8 expand_rgb5_to_rgb8(UINT8 val)
{
	UINT8 temp = val & 0x1f;
	return (temp << 3) | (temp >> 2);
}

// Perform a lerp between a and b
INLINE UINT8 vii_mix_channel(vii_state *state, UINT8 a, UINT8 b)
{
	UINT8 alpha = state->m_video_regs[0x1c] & 0x00ff;
	return ((64 - alpha) * a + alpha * b) / 64;
}

static void vii_mix_pixel(vii_state *state, UINT32 offset, UINT16 rgb)
{
	state->m_screen[offset].r = vii_mix_channel(state, state->m_screen[offset].r, expand_rgb5_to_rgb8(rgb >> 10));
	state->m_screen[offset].g = vii_mix_channel(state, state->m_screen[offset].g, expand_rgb5_to_rgb8(rgb >> 5));
	state->m_screen[offset].b = vii_mix_channel(state, state->m_screen[offset].b, expand_rgb5_to_rgb8(rgb));
}

static void vii_set_pixel(vii_state *state, UINT32 offset, UINT16 rgb)
{
	state->m_screen[offset].r = expand_rgb5_to_rgb8(rgb >> 10);
	state->m_screen[offset].g = expand_rgb5_to_rgb8(rgb >> 5);
	state->m_screen[offset].b = expand_rgb5_to_rgb8(rgb);
}

static void vii_blit(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, UINT32 xoff, UINT32 yoff, UINT32 attr, UINT32 ctrl, UINT32 bitmap_addr, UINT16 tile)
{
	vii_state *state = machine.driver_data<vii_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	UINT32 h = 8 << ((attr & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
	UINT32 w = 8 << ((attr & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

	UINT32 yflipmask = attr & TILE_Y_FLIP ? h - 1 : 0;
	UINT32 xflipmask = attr & TILE_X_FLIP ? w - 1 : 0;

	UINT32 nc = ((attr & 0x0003) + 1) << 1;

	UINT32 palette_offset = (attr & 0x0f00) >> 4;
	palette_offset >>= nc;
	palette_offset <<= nc;

	UINT32 m = bitmap_addr + nc*w*h/16*tile;
	UINT32 bits = 0;
	UINT32 nbits = 0;

	UINT32 x, y;

	for(y = 0; y < h; y++)
	{
		UINT32 yy = (yoff + (y ^ yflipmask)) & 0x1ff;

		for(x = 0; x < w; x++)
		{
			UINT32 xx = (xoff + (x ^ xflipmask)) & 0x1ff;
			UINT32 pal;

			bits <<= nc;
			if(nbits < nc)
			{
				UINT16 b = space->read_word((m++ & 0x3fffff) << 1);
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			pal = palette_offset | (bits >> 16);
			bits &= 0xffff;

			if((ctrl & 0x0010) && yy < 240)
			{
				xx = (xx - (INT16)state->m_rowscroll[yy]) & 0x01ff;
			}

			if(xx < 320 && yy < 240)
			{
				UINT16 rgb = state->m_palette[pal];
				if(!(rgb & 0x8000))
				{
					if (attr & 0x4000)
					{
						vii_mix_pixel(state, xx + 320*yy, rgb);
					}
					else
					{
						vii_set_pixel(state, xx + 320*yy, rgb);
					}
				}
			}
		}
	}
}

static void vii_blit_page(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, int depth, UINT32 bitmap_addr, UINT16 *regs)
{
	UINT32 x0, y0;
	UINT32 xscroll = regs[0];
	UINT32 yscroll = regs[1];
	UINT32 attr = regs[2];
	UINT32 ctrl = regs[3];
	UINT32 tilemap = regs[4];
	UINT32 palette_map = regs[5];
	UINT32 h, w, hn, wn;
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	if(!(ctrl & PAGE_ENABLE_MASK))
	{
		return;
	}

	if(((attr & PAGE_DEPTH_FLAG_MASK) >> PAGE_DEPTH_FLAG_SHIFT) != depth)
	{
		return;
	}

	h = 8 << ((attr & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
	w = 8 << ((attr & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

	hn = 256 / h;
	wn = 512 / w;

	for(y0 = 0; y0 < hn; y0++)
	{
		for(x0 = 0; x0 < wn; x0++)
		{
			UINT16 tile = space->read_word((tilemap + x0 + wn * y0) << 1);
			UINT16 palette = 0;
			UINT32 xx, yy;

			if(!tile)
			{
				continue;
			}

			palette = space->read_word((palette_map + (x0 + wn * y0) / 2) << 1);
			if(x0 & 1)
			{
				palette >>= 8;
			}

			UINT32 tileattr = attr;
			UINT32 tilectrl = ctrl;
			if ((ctrl & 2) == 0)
			{	// -(1) bld(1) flip(2) pal(4)
				tileattr &= ~0x000c;
				tileattr |= (palette >> 2) & 0x000c;	// flip

				tileattr &= ~0x0f00;
				tileattr |= (palette << 8) & 0x0f00;	// palette

				tileattr &= ~0x0100;
				tileattr |= (palette << 2) & 0x0100;	// blend
			}

			yy = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			xx = (w*x0 - xscroll) & 0x1ff;

			vii_blit(machine, bitmap, cliprect, xx, yy, tileattr, tilectrl, bitmap_addr, tile);
		}
	}
}

static void vii_blit_sprite(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, int depth, UINT32 base_addr)
{
	vii_state *state = machine.driver_data<vii_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	UINT16 tile, attr;
	INT16 x, y;
	UINT32 h, w;
	UINT32 bitmap_addr = 0x40 * state->m_video_regs[0x22];

	tile = space->read_word((base_addr + 0) << 1);
	x = space->read_word((base_addr + 1) << 1);
	y = space->read_word((base_addr + 2) << 1);
	attr = space->read_word((base_addr + 3) << 1);

	if(!tile)
	{
		return;
	}

	if(((attr & PAGE_DEPTH_FLAG_MASK) >> PAGE_DEPTH_FLAG_SHIFT) != depth)
	{
		return;
	}

	if(state->m_centered_coordinates)
	{
		x = 160 + x;
		y = 120 - y;

		h = 8 << ((attr & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
		w = 8 << ((attr & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

		x -= (w/2);
		y -= (h/2) - 8;
	}

	x &= 0x01ff;
	y &= 0x01ff;

	vii_blit(machine, bitmap, cliprect, x, y, attr, 0, bitmap_addr, tile);
}

static void vii_blit_sprites(running_machine &machine, bitmap_t *bitmap, const rectangle *cliprect, int depth)
{
	vii_state *state = machine.driver_data<vii_state>();
	UINT32 n;

	if (!(state->m_video_regs[0x42] & 1))
	{
		return;
	}

	for(n = 0; n < 256; n++)
	{
		//if(space->read_word((0x2c00 + 4*n) << 1))
		{
			vii_blit_sprite(machine, bitmap, cliprect, depth, 0x2c00 + 4*n);
		}
	}
}

static SCREEN_UPDATE( vii )
{
	vii_state *state = screen->machine().driver_data<vii_state>();
	int i, x, y;

	bitmap_fill(bitmap, cliprect, 0);

	memset(state->m_screen, 0, sizeof(state->m_screen));

	for(i = 0; i < 4; i++)
	{
		vii_blit_page(screen->machine(), bitmap, cliprect, i, 0x40 * state->m_video_regs[0x20], state->m_video_regs + 0x10);
		vii_blit_page(screen->machine(), bitmap, cliprect, i, 0x40 * state->m_video_regs[0x21], state->m_video_regs + 0x16);
		vii_blit_sprites(screen->machine(), bitmap, cliprect, i);
	}

	for(y = 0; y < 240; y++)
	{
		for(x = 0; x < 320; x++)
		{
			*BITMAP_ADDR32(bitmap, y, x) = (state->m_screen[x + 320*y].r << 16) | (state->m_screen[x + 320*y].g << 8) | state->m_screen[x + 320*y].b;
		}
	}

	return 0;
}

/*************************
*    Machine Hardware    *
*************************/

static void vii_do_dma(running_machine &machine, UINT32 len)
{
	vii_state *state = machine.driver_data<vii_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	UINT32 src = state->m_video_regs[0x70];
	UINT32 dst = state->m_video_regs[0x71] + 0x2c00;
	UINT32 j;

	for(j = 0; j < len; j++)
	{
		space->write_word((dst+j) << 1, space->read_word((src+j) << 1));
	}

	state->m_video_regs[0x72] = 0;
}

static READ16_HANDLER( vii_video_r )
{
	vii_state *state = space->machine().driver_data<vii_state>();

	switch(offset)
	{
		case 0x62: // Video IRQ Enable
			verboselog(space->machine(), 0, "vii_video_r: Video IRQ Enable: %04x\n", VII_VIDEO_IRQ_ENABLE);
			return VII_VIDEO_IRQ_ENABLE;

		case 0x63: // Video IRQ Status
			verboselog(space->machine(), 0, "vii_video_r: Video IRQ Status: %04x\n", VII_VIDEO_IRQ_STATUS);
			return VII_VIDEO_IRQ_STATUS;

		default:
			verboselog(space->machine(), 0, "vii_video_r: Unknown register %04x = %04x\n", 0x2800 + offset, state->m_video_regs[offset]);
			break;
	}
	return state->m_video_regs[offset];
}

static WRITE16_HANDLER( vii_video_w )
{
	vii_state *state = space->machine().driver_data<vii_state>();

	switch(offset)
	{
		case 0x62: // Video IRQ Enable
			verboselog(space->machine(), 0, "vii_video_w: Video IRQ Enable = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&VII_VIDEO_IRQ_ENABLE);
			break;

		case 0x63: // Video IRQ Acknowledge
			verboselog(space->machine(), 0, "vii_video_w: Video IRQ Acknowledge = %04x (%04x)\n", data, mem_mask);
			VII_VIDEO_IRQ_STATUS &= ~data;
			if(!VII_VIDEO_IRQ_STATUS)
			{
				cputag_set_input_line(space->machine(), "maincpu", UNSP_IRQ0_LINE, CLEAR_LINE);
			}
			break;

		case 0x70: // Video DMA Source
			verboselog(space->machine(), 0, "vii_video_w: Video DMA Source = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_video_regs[offset]);
			break;

		case 0x71: // Video DMA Dest
			verboselog(space->machine(), 0, "vii_video_w: Video DMA Dest = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_video_regs[offset]);
			break;

		case 0x72: // Video DMA Length
			verboselog(space->machine(), 0, "vii_video_w: Video DMA Length = %04x (%04x)\n", data, mem_mask);
			vii_do_dma(space->machine(), data);
			break;

		default:
			verboselog(space->machine(), 0, "vii_video_w: Unknown register %04x = %04x (%04x)\n", 0x2800 + offset, data, mem_mask);
			COMBINE_DATA(&state->m_video_regs[offset]);
			break;
	}
}

static READ16_HANDLER( vii_audio_r )
{
	switch(offset)
	{
		default:
			verboselog(space->machine(), 4, "vii_audio_r: Unknown register %04x\n", 0x3000 + offset);
			break;
	}
	return 0;
}

static WRITE16_HANDLER( vii_audio_w )
{
	switch(offset)
	{
		default:
			verboselog(space->machine(), 4, "vii_audio_w: Unknown register %04x = %04x (%04x)\n", 0x3000 + offset, data, mem_mask);
			break;
	}
}

static void vii_switch_bank(running_machine &machine, UINT32 bank)
{
	vii_state *state = machine.driver_data<vii_state>();
	UINT8 *cart = machine.region("cart")->base();

	if(bank == state->m_current_bank)
	{
		return;
	}

	state->m_current_bank = bank;

	memcpy(state->m_cart, cart + 0x400000 * bank * 2 + 0x4000*2, (0x400000 - 0x4000) * 2);
}

static void vii_do_gpio(running_machine &machine, UINT32 offset)
{
	vii_state *state = machine.driver_data<vii_state>();
	UINT32 index  = (offset - 1) / 5;
	UINT16 buffer = state->m_io_regs[5*index + 2];
	UINT16 dir    = state->m_io_regs[5*index + 3];
	UINT16 attr   = state->m_io_regs[5*index + 4];
	UINT16 special= state->m_io_regs[5*index + 5];

	UINT16 push   = dir;
	UINT16 pull   = (~dir) & (~attr);
	UINT16 what   = (buffer & (push | pull));
	what ^= (dir & ~attr);
	what &= ~special;

	if(state->m_spg243_mode == SPG243_VII)
	{
		if(index == 1)
		{
			UINT32 bank = ((what & 0x80) >> 7) | ((what & 0x20) >> 4);
			vii_switch_bank(machine, bank);
		}
	}
	else if(state->m_spg243_mode == SPG243_BATMAN)
	{
		if(index == 0)
		{
			UINT16 temp = input_port_read(machine, "P1");
			what |= (temp & 0x0001) ? 0x8000 : 0;
			what |= (temp & 0x0002) ? 0x4000 : 0;
			what |= (temp & 0x0004) ? 0x2000 : 0;
			what |= (temp & 0x0008) ? 0x1000 : 0;
			what |= (temp & 0x0010) ? 0x0800 : 0;
			what |= (temp & 0x0020) ? 0x0400 : 0;
			what |= (temp & 0x0040) ? 0x0200 : 0;
			what |= (temp & 0x0040) ? 0x0100 : 0;
		}

		if(index == 2)
		{
		}
	}

	state->m_io_regs[5*index + 1] = what;
}

static void vii_do_i2c(running_machine &machine)
{
}

static void spg_do_dma(running_machine &machine, UINT32 len)
{
	vii_state *state = machine.driver_data<vii_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	UINT32 src = ((state->m_io_regs[0x101] & 0x3f) << 16) | state->m_io_regs[0x100];
	UINT32 dst = state->m_io_regs[0x103] & 0x3fff;
	UINT32 j;

	for(j = 0; j < len; j++)
	{
		space->write_word((dst+j) << 1, space->read_word((src+j) << 1));
	}

	state->m_io_regs[0x102] = 0;
}

static READ16_HANDLER( vii_io_r )
{
	static const char *const gpioregs[] = { "GPIO Data Port", "GPIO Buffer Port", "GPIO Direction Port", "GPIO Attribute Port", "GPIO IRQ/Latch Port" };
	static const char gpioports[] = { 'A', 'B', 'C' };

	vii_state *state = space->machine().driver_data<vii_state>();
	UINT16 val = state->m_io_regs[offset];

	offset -= 0x500;

	switch(offset)
	{
		case 0x01: case 0x06: case 0x0b: // GPIO Data Port A/B/C
			vii_do_gpio(space->machine(), offset);
			verboselog(space->machine(), 3, "vii_io_r: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], state->m_io_regs[offset], mem_mask);
			val = state->m_io_regs[offset];
			break;

		case 0x02: case 0x03: case 0x04: case 0x05:
		case 0x07: case 0x08: case 0x09: case 0x0a:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // Other GPIO regs
			verboselog(space->machine(), 3, "vii_io_r: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], state->m_io_regs[offset], mem_mask);
			break;

		case 0x1c: // Random
			val = space->machine().rand() & 0x00ff;
			verboselog(space->machine(), 3, "vii_io_r: Random = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x22: // IRQ Status
			val = state->m_io_regs[0x21];
			verboselog(space->machine(), 3, "vii_io_r: Controller IRQ Status = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x2c: case 0x2d: // Timers?
			val = space->machine().rand() & 0x0000ffff;
			verboselog(space->machine(), 3, "vii_io_r: Unknown Timer %d Register = %04x (%04x)\n", offset - 0x2c, val, mem_mask);
			break;

		case 0x2f: // Data Segment
			val = cpu_get_reg(space->machine().device("maincpu"), UNSP_SR) >> 10;
			verboselog(space->machine(), 3, "vii_io_r: Data Segment = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x31: // Unknown, UART Status?
			verboselog(space->machine(), 3, "vii_io_r: Unknown (UART Status?) = %04x (%04x)\n", 3, mem_mask);
			val = 3;
			break;

		case 0x36: // UART RX Data
			val = state->m_controller_input[state->m_uart_rx_count];
			state->m_uart_rx_count = (state->m_uart_rx_count + 1) % 8;
			verboselog(space->machine(), 3, "vii_io_r: UART RX Data = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x59: // I2C Status
			verboselog(space->machine(), 3, "vii_io_r: I2C Status = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x5e: // I2C Data In
			verboselog(space->machine(), 3, "vii_io_r: I2C Data In = %04x (%04x)\n", val, mem_mask);
			break;

		default:
			verboselog(space->machine(), 3, "vii_io_r: Unknown register %04x\n", 0x3800 + offset);
			break;
	}

	return val;
}

static WRITE16_HANDLER( vii_io_w )
{
	static const char *const gpioregs[] = { "GPIO Data Port", "GPIO Buffer Port", "GPIO Direction Port", "GPIO Attribute Port", "GPIO IRQ/Latch Port" };
	static const char gpioports[3] = { 'A', 'B', 'C' };

	vii_state *state = space->machine().driver_data<vii_state>();
	UINT16 temp = 0;

	offset -= 0x500;

	switch(offset)
	{
		case 0x00: // GPIO special function select
			verboselog(space->machine(), 3, "vii_io_w: GPIO Function Select = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x01: case 0x06: case 0x0b: // GPIO data, port A/B/C
			offset++;
			// Intentional fallthrough

		case 0x02: case 0x03: case 0x04: case 0x05: // Port A
		case 0x07: case 0x08: case 0x09: case 0x0a: // Port B
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // Port C
			verboselog(space->machine(), 3, "vii_io_w: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			vii_do_gpio(space->machine(), offset);
			break;

		case 0x21: // IRQ Enable
			verboselog(space->machine(), 3, "vii_io_w: Controller IRQ Enable = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&VII_CTLR_IRQ_ENABLE);
			if(!VII_CTLR_IRQ_ENABLE)
			{
				cputag_set_input_line(space->machine(), "maincpu", UNSP_IRQ3_LINE, CLEAR_LINE);
			}
			break;

		case 0x22: // IRQ Acknowledge
			verboselog(space->machine(), 3, "vii_io_w: Controller IRQ Acknowledge = %04x (%04x)\n", data, mem_mask);
			state->m_io_regs[0x22] &= ~data;
			if(!state->m_io_regs[0x22])
			{
				cputag_set_input_line(space->machine(), "maincpu", UNSP_IRQ3_LINE, CLEAR_LINE);
			}
			break;

		case 0x2f: // Data Segment
			temp = cpu_get_reg(space->machine().device("maincpu"), UNSP_SR);
			cpu_set_reg(space->machine().device("maincpu"), UNSP_SR, (temp & 0x03ff) | ((data & 0x3f) << 10));
			verboselog(space->machine(), 3, "vii_io_w: Data Segment = %04x (%04x)\n", data, mem_mask);
			break;

		case 0x31: // Unknown UART
			verboselog(space->machine(), 3, "vii_io_w: Unknown UART = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x32: // UART Reset
			verboselog(space->machine(), 3, "vii_io_r: UART Reset\n");
			break;

		case 0x33: // UART Baud Rate
			verboselog(space->machine(), 3, "vii_io_w: UART Baud Rate = %u\n", 27000000 / 16 / (0x10000 - (state->m_io_regs[0x34] << 8) - data));
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x35: // UART TX Data
			verboselog(space->machine(), 3, "vii_io_w: UART Baud Rate = %u\n", 27000000 / 16 / (0x10000 - (data << 8) - state->m_io_regs[0x33]));
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5a: // I2C Access Mode
			verboselog(space->machine(), 3, "vii_io_w: I2C Access Mode = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5b: // I2C Device Address
			verboselog(space->machine(), 3, "vii_io_w: I2C Device Address = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5c: // I2C Sub-Address
			verboselog(space->machine(), 3, "vii_io_w: I2C Sub-Address = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5d: // I2C Data Out
			verboselog(space->machine(), 3, "vii_io_w: I2C Data Out = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5e: // I2C Data In
			verboselog(space->machine(), 3, "vii_io_w: I2C Data In = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x5f: // I2C Controller Mode
			verboselog(space->machine(), 3, "vii_io_w: I2C Controller Mode = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x58: // I2C Command
			verboselog(space->machine(), 3, "vii_io_w: I2C Command = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			vii_do_i2c(space->machine());
			break;

		case 0x59: // I2C Status / IRQ Acknowledge(?)
			verboselog(space->machine(), 3, "vii_io_w: I2C Status / Ack = %04x (%04x)\n", data, mem_mask);
			state->m_io_regs[offset] &= ~data;
			break;

		case 0x100: // DMA Source (L)
		case 0x101: // DMA Source (H)
		case 0x103: // DMA Destination
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;

		case 0x102: // DMA Length
			spg_do_dma(space->machine(), data);
			break;

		default:
			verboselog(space->machine(), 3, "vii_io_w: Unknown register %04x = %04x (%04x)\n", 0x3800 + offset, data, mem_mask);
			COMBINE_DATA(&state->m_io_regs[offset]);
			break;
	}
}

/*
static WRITE16_HANDLER( vii_rowscroll_w )
{
    switch(offset)
    {
        default:
            verboselog(space->machine(), 0, "vii_rowscroll_w: %04x = %04x (%04x)\n", 0x2900 + offset, data, mem_mask);
            break;
    }
}

static WRITE16_HANDLER( vii_spriteram_w )
{
    switch(offset)
    {
        default:
            verboselog(space->machine(), 0, "vii_spriteram_w: %04x = %04x (%04x)\n", 0x2c00 + offset, data, mem_mask);
            break;
    }
}
*/

static ADDRESS_MAP_START( vii_mem, AS_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x004fff ) AM_RAM AM_BASE_MEMBER(vii_state,m_ram)
	AM_RANGE( 0x005000, 0x0051ff ) AM_READWRITE(vii_video_r, vii_video_w)
	AM_RANGE( 0x005200, 0x0055ff ) AM_RAM AM_BASE_MEMBER(vii_state,m_rowscroll)
	AM_RANGE( 0x005600, 0x0057ff ) AM_RAM AM_BASE_MEMBER(vii_state,m_palette)
	AM_RANGE( 0x005800, 0x005fff ) AM_RAM AM_BASE_MEMBER(vii_state,m_spriteram)
	AM_RANGE( 0x006000, 0x006fff ) AM_READWRITE(vii_audio_r, vii_audio_w)
	AM_RANGE( 0x007000, 0x007fff ) AM_READWRITE(vii_io_r,    vii_io_w)
	AM_RANGE( 0x008000, 0x7fffff ) AM_ROM AM_BASE_MEMBER(vii_state,m_cart)
ADDRESS_MAP_END

static INPUT_PORTS_START( vii )
	PORT_START("P1")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_NAME("Joypad Up")
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_NAME("Joypad Down")
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_NAME("Joypad Left")
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("Joypad Right")
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_PLAYER(1) PORT_NAME("Button A")
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )        PORT_PLAYER(1) PORT_NAME("Button B")
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )        PORT_PLAYER(1) PORT_NAME("Button C")
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )		 PORT_PLAYER(1) PORT_NAME("Button D")
INPUT_PORTS_END

static INPUT_PORTS_START( batman )
	PORT_START("P1")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_NAME("Joypad Up")
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_NAME("Joypad Down")
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_NAME("Joypad Left")
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("Joypad Right")
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_PLAYER(1) PORT_NAME("A Button")
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )        PORT_PLAYER(1) PORT_NAME("Menu")
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )        PORT_PLAYER(1) PORT_NAME("B Button")
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )		 PORT_PLAYER(1) PORT_NAME("X Button")
INPUT_PORTS_END

static INPUT_PORTS_START( vsmile )
	PORT_START("P1")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_NAME("Joypad Up")
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_NAME("Joypad Down")
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_NAME("Joypad Left")
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("Joypad Right")
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_PLAYER(1) PORT_NAME("A Button")
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )        PORT_PLAYER(1) PORT_NAME("Menu")
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )        PORT_PLAYER(1) PORT_NAME("B Button")
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )		 PORT_PLAYER(1) PORT_NAME("X Button")
INPUT_PORTS_END

static DEVICE_IMAGE_LOAD( vii_cart )
{
	vii_state *state = image.device().machine().driver_data<vii_state>();
	UINT8 *cart = image.device().machine().region( "cart" )->base();
	if (image.software_entry() == NULL)
	{
		int size = image.length();

		if( image.fread(cart, size ) != size )
		{
			image.seterror( IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
			return IMAGE_INIT_FAIL;
		}
	} else {
		int filesize = image.get_software_region_length("rom");
		memcpy(cart, image.get_software_region("rom"), filesize);
	}

	memcpy(state->m_cart, cart + 0x4000*2, (0x400000 - 0x4000) * 2);

	if( cart[0x3cd808] == 0x99 &&
		cart[0x3cd809] == 0x99 &&
		cart[0x3cd80a] == 0x83 &&
		cart[0x3cd80b] == 0x5e &&
		cart[0x3cd80c] == 0x52 &&
		cart[0x3cd80d] == 0x6b &&
		cart[0x3cd80e] == 0x78 &&
		cart[0x3cd80f] == 0x7f )
	{
		state->m_centered_coordinates = 0;
	}
	return IMAGE_INIT_PASS;
}

static DEVICE_IMAGE_LOAD( vsmile_cart )
{
	vii_state *state = image.device().machine().driver_data<vii_state>();
	UINT8 *cart = image.device().machine().region( "cart" )->base();
	if (image.software_entry() == NULL)
	{
		int size = image.length();

		if( image.fread( cart, size ) != size )
		{
			image.seterror( IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
			return IMAGE_INIT_FAIL;
		}
	}
	else
	{
		int filesize = image.get_software_region_length("rom");
		memcpy(cart, image.get_software_region("rom"), filesize);
	}
	memcpy(state->m_cart, cart + 0x4000*2, (0x400000 - 0x4000) * 2);
	return IMAGE_INIT_PASS;
}

static MACHINE_START( vii )
{
	vii_state *state = machine.driver_data<vii_state>();

	memset(state->m_video_regs, 0, 0x100 * sizeof(UINT16));
	memset(state->m_io_regs, 0, 0x100 * sizeof(UINT16));
	state->m_current_bank = 0;

	state->m_controller_input[0] = 0;
	state->m_controller_input[4] = 0;
	state->m_controller_input[6] = 0xff;
	state->m_controller_input[7] = 0;

	UINT8 *rom = machine.region( "cart" )->base();
	if (rom) { // to prevent batman crash
		memcpy(state->m_cart, rom + 0x4000*2, (0x400000 - 0x4000) * 2);
	}
}

static MACHINE_RESET( vii )
{
}

static INTERRUPT_GEN( vii_vblank )
{
	vii_state *state = device->machine().driver_data<vii_state>();
	UINT32 x = device->machine().rand() & 0x3ff;
	UINT32 y = device->machine().rand() & 0x3ff;
	UINT32 z = device->machine().rand() & 0x3ff;

	VII_VIDEO_IRQ_STATUS = VII_VIDEO_IRQ_ENABLE & 1;
	if(VII_VIDEO_IRQ_STATUS)
	{
		verboselog(device->machine(), 0, "Video IRQ\n");
		cputag_set_input_line(device->machine(), "maincpu", UNSP_IRQ0_LINE, ASSERT_LINE);
	}

	state->m_controller_input[0] = input_port_read(device->machine(), "P1");
	state->m_controller_input[1] = (UINT8)x;
	state->m_controller_input[2] = (UINT8)y;
	state->m_controller_input[3] = (UINT8)z;
	state->m_controller_input[4] = 0;
	x >>= 8;
	y >>= 8;
	z >>= 8;
	state->m_controller_input[5] = (z << 4) | (y << 2) | x;
	state->m_controller_input[6] = 0xff;
	state->m_controller_input[7] = 0;

	state->m_uart_rx_count = 0;

	if(VII_CTLR_IRQ_ENABLE)
	{
		verboselog(device->machine(), 0, "Controller IRQ\n");
		cputag_set_input_line(device->machine(), "maincpu", UNSP_IRQ3_LINE, ASSERT_LINE);
	}
}

static MACHINE_CONFIG_START( vii, vii_state )

	MCFG_CPU_ADD( "maincpu", UNSP, XTAL_27MHz)
	MCFG_CPU_PROGRAM_MAP( vii_mem )
	MCFG_CPU_VBLANK_INT("screen", vii_vblank)

	MCFG_MACHINE_START( vii )
	MCFG_MACHINE_RESET( vii )

	MCFG_SCREEN_ADD( "screen", RASTER )
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MCFG_SCREEN_UPDATE( vii )

	MCFG_PALETTE_LENGTH(32768)

	MCFG_CARTSLOT_ADD( "cart" )
	MCFG_CARTSLOT_EXTENSION_LIST( "bin" )
	MCFG_CARTSLOT_LOAD( vii_cart )
	MCFG_CARTSLOT_INTERFACE("vii_cart")

	MCFG_VIDEO_START( vii )

	MCFG_SOFTWARE_LIST_ADD("vii_cart","vii")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( vsmile, vii_state )

	MCFG_CPU_ADD( "maincpu", UNSP, XTAL_27MHz)
	MCFG_CPU_PROGRAM_MAP( vii_mem )
	MCFG_CPU_VBLANK_INT("screen", vii_vblank)

	MCFG_MACHINE_START( vii )
	MCFG_MACHINE_RESET( vii )

	MCFG_SCREEN_ADD( "screen", RASTER )
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MCFG_SCREEN_UPDATE( vii )

	MCFG_PALETTE_LENGTH(32768)

	MCFG_CARTSLOT_ADD( "cart" )
	MCFG_CARTSLOT_EXTENSION_LIST( "bin" )
	MCFG_CARTSLOT_MANDATORY
	MCFG_CARTSLOT_LOAD( vsmile_cart )

	MCFG_VIDEO_START( vii )
MACHINE_CONFIG_END

static const i2cmem_interface i2cmem_interface =
{
       I2CMEM_SLAVE_ADDRESS, 0, 0x200
};

static MACHINE_CONFIG_START( batman, vii_state )

	MCFG_CPU_ADD( "maincpu", UNSP, XTAL_27MHz)
	MCFG_CPU_PROGRAM_MAP( vii_mem )
	MCFG_CPU_VBLANK_INT("screen", vii_vblank)

	MCFG_MACHINE_START( vii )
	MCFG_MACHINE_RESET( vii )

	MCFG_I2CMEM_ADD("i2cmem",i2cmem_interface)

	MCFG_SCREEN_ADD( "screen", RASTER )
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MCFG_SCREEN_UPDATE( vii )

	MCFG_PALETTE_LENGTH(32768)

	MCFG_VIDEO_START( vii )
MACHINE_CONFIG_END

static DRIVER_INIT( vii )
{
	vii_state *state = machine.driver_data<vii_state>();

	state->m_spg243_mode = SPG243_VII;
	state->m_centered_coordinates = 1;
}

static DRIVER_INIT( batman )
{
	vii_state *state = machine.driver_data<vii_state>();

	state->m_spg243_mode = SPG243_BATMAN;
	state->m_centered_coordinates = 1;
}

static DRIVER_INIT( vsmile )
{
	vii_state *state = machine.driver_data<vii_state>();

	state->m_spg243_mode = SPG243_VSMILE;
	state->m_centered_coordinates = 1;
}

ROM_START( vii )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for u'nSP */

	ROM_REGION( 0x2000000, "cart", ROMREGION_ERASE00 )
	ROM_LOAD( "vii.bin", 0x0000, 0x2000000, CRC(04627639) SHA1(f883a92d31b53c9a5b0cdb112d07cd793c95fc43))
ROM_END

ROM_START( batmantv )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for u'nSP */
	ROM_LOAD16_WORD_SWAP( "batman.bin", 0x000000, 0x400000, CRC(46f848e5) SHA1(5875d57bb3fe0cac5d20e626e4f82a0e5f9bb94c) )
ROM_END

ROM_START( vsmile )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for u'nSP */

	ROM_REGION( 0x2000000, "cart", ROMREGION_ERASE00 )
ROM_END

/*    YEAR  NAME     PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY                                              FULLNAME      FLAGS */
CONS( 2004, batmantv, 0,        0,        batman,   batman,   batman,   "JAKKS Pacific Inc / HotGen Ltd",                "The Batman", GAME_NO_SOUND )
CONS( 2005, vsmile,  0,        0,        vsmile,   vsmile,   vsmile,   "V-Tech",                                            "V-Smile",    GAME_NO_SOUND | GAME_NOT_WORKING )
CONS( 2007, vii,     0,        0,        vii,      vii,      vii,      "Jungle Soft / KenSingTon / Chintendo / Siatronics", "Vii",        GAME_NO_SOUND )
