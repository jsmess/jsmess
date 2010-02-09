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
        ND - Likely many more

*******************************************************************************/

#include "emu.h"
#include "cpu/unsp/unsp.h"
#include "devices/cartslot.h"
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

typedef struct _vii_state vii_state;
struct _vii_state
{
	UINT16 *ram;
	UINT16 *cart;
	UINT16 *rowscroll;
	UINT16 *palette;
	UINT16 *spriteram;

	UINT32 current_bank;

	UINT16 video_regs[0x100];
	UINT32 centered_coordinates;

	struct
	{
		UINT8 r, g, b;
	}
	screen[320*240];

	UINT16 io_regs[0x100];
	UINT16 uart_rx_count;
	UINT8 controller_input[8];
	UINT32 spg243_mode;
};

enum
{
	SPG243_VII = 0,
	SPG243_BATMAN,

	SPG243_MODEL_COUNT,
};


#define VII_CTLR_IRQ_ENABLE		state->io_regs[0x21]
#define VII_VIDEO_IRQ_ENABLE	state->video_regs[0x62]
#define VII_VIDEO_IRQ_STATUS	state->video_regs[0x63]


#define VERBOSE_LEVEL	(3)

#define ENABLE_VERBOSE_LOG (1)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%04x: %s", cpu_get_pc(devtag_get_device(machine, "maincpu")), buf );
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
	UINT8 alpha = state->video_regs[0x1c] & 0x00ff;
	return ((64 - alpha) * a + alpha * b) / 64;
}

static void vii_mix_pixel(vii_state *state, UINT32 offset, UINT16 rgb)
{
	state->screen[offset].r = vii_mix_channel(state, state->screen[offset].r, expand_rgb5_to_rgb8(rgb >> 10));
	state->screen[offset].g = vii_mix_channel(state, state->screen[offset].g, expand_rgb5_to_rgb8(rgb >> 5));
	state->screen[offset].b = vii_mix_channel(state, state->screen[offset].b, expand_rgb5_to_rgb8(rgb));
}

static void vii_set_pixel(vii_state *state, UINT32 offset, UINT16 rgb)
{
	state->screen[offset].r = expand_rgb5_to_rgb8(rgb >> 10);
	state->screen[offset].g = expand_rgb5_to_rgb8(rgb >> 5);
	state->screen[offset].b = expand_rgb5_to_rgb8(rgb);
}

static void vii_blit(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, UINT32 xoff, UINT32 yoff, UINT32 flags, UINT32 bitmap_addr, UINT16 tile)
{
	vii_state *state = (vii_state *)machine->driver_data;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	UINT32 h = 8 << ((flags & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
	UINT32 w = 8 << ((flags & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

	UINT32 yflipmask = flags & TILE_Y_FLIP ? h - 1 : 0;
	UINT32 xflipmask = flags & TILE_X_FLIP ? w - 1 : 0;

	UINT32 nc = ((flags & 0x0003) + 1) << 1;

	UINT32 palette_offset = (flags & 0x0f00) >> 4;

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
				UINT16 b = memory_read_word_16le(space, (m++ & 0x3fffff) << 1);
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			pal = palette_offset + (bits >> 16);
			bits &= 0xffff;

			if((flags & 0x00100000) && yy < 240)
			{
				xx = (xx - (INT16)state->rowscroll[yy]) & 0x01ff;
			}

			if(xx < 320 && yy < 240)
			{
				UINT16 rgb = state->palette[pal];
				if(!(rgb & 0x8000))
				{
					if (flags & 0x4000)
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

static void vii_blit_page(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int depth, UINT32 bitmap_addr, UINT16 *regs)
{
	UINT32 x0, y0;
	UINT32 xscroll = regs[0];
	UINT32 yscroll = regs[1];
	UINT32 flags = regs[2];
	UINT32 flags2 = regs[3];
	UINT32 tilemap = regs[4];
	UINT32 palette_map = regs[5];
	UINT32 h, w, hn, wn;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if(!(flags2 & PAGE_ENABLE_MASK))
	{
		return;
	}

	if(((flags & PAGE_DEPTH_FLAG_MASK) >> PAGE_DEPTH_FLAG_SHIFT) != depth)
	{
		return;
	}

	h = 8 << ((flags & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
	w = 8 << ((flags & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

	hn = 256 / h;
	wn = 512 / w;

	for(y0 = 0; y0 < hn; y0++)
	{
		for(x0 = 0; x0 < wn; x0++)
		{
			UINT16 tile = memory_read_word_16le(space, (tilemap + x0 + wn * y0) << 1);
			UINT16 palette = 0;
			UINT32 tileflags = 0;
			UINT32 xx, yy;

			if(!tile)
			{
				continue;
			}

			palette = memory_read_word_16le(space, (palette_map + (x0 + wn * y0) / 2) << 1);
			if(x0 & 1)
			{
				palette >>= 8;
			}

			if(palette & 0x10) // X flip
			{
				tileflags |= 4;
			}
			tileflags |= (palette & 0x0f) << 8;

			yy = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			xx = (w*x0 - xscroll) & 0x1ff;

			vii_blit(machine, bitmap, cliprect, xx, yy, (flags2 << 16) | flags | tileflags, bitmap_addr, tile);
		}
	}
}

static void vii_blit_sprite(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int depth, UINT32 base_addr)
{
	vii_state *state = (vii_state *)machine->driver_data;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 tile, flags;
	INT16 x, y;
	UINT32 h, w;
	UINT32 bitmap_addr = 0x40 * state->video_regs[0x22];

	tile = memory_read_word_16le(space, (base_addr + 0) << 1);
	x = memory_read_word_16le(space, (base_addr + 1) << 1);
	y = memory_read_word_16le(space, (base_addr + 2) << 1);
	flags = memory_read_word_16le(space, (base_addr + 3) << 1);

	if(((flags & PAGE_DEPTH_FLAG_MASK) >> PAGE_DEPTH_FLAG_SHIFT) != depth)
	{
		return;
	}

	if(state->centered_coordinates)
	{
		x = 160 + x;
		y = 120 - y;

		h = 8 << ((flags & PAGE_TILE_HEIGHT_MASK) >> PAGE_TILE_HEIGHT_SHIFT);
		w = 8 << ((flags & PAGE_TILE_WIDTH_MASK) >> PAGE_TILE_WIDTH_SHIFT);

		x -= (w/2);
		y -= (h/2) - 8;
	}

	x &= 0x01ff;
	y &= 0x01ff;

	vii_blit(machine, bitmap, cliprect, x, y, flags, bitmap_addr, tile);
}

static void vii_blit_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int depth)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 n;
	for(n = 0; n < 256; n++)
	{
		if(memory_read_word_16le(space, (0x2c00 + 4*n) << 1))
		{
			vii_blit_sprite(machine, bitmap, cliprect, depth, 0x2c00 + 4*n);
		}
	}
}

static VIDEO_UPDATE( vii )
{
	vii_state *state = (vii_state *)screen->machine->driver_data;
	int i, x, y;

	bitmap_fill(bitmap, cliprect, 0);

	memset(state->screen, 0, sizeof(state->screen));

	for(i = 0; i < 4; i++)
	{
		vii_blit_page(screen->machine, bitmap, cliprect, i, 0x40 * state->video_regs[0x20], state->video_regs + 0x10);
		vii_blit_page(screen->machine, bitmap, cliprect, i, 0x40 * state->video_regs[0x21], state->video_regs + 0x16);
		vii_blit_sprites(screen->machine, bitmap, cliprect, i);
	}

	for(y = 0; y < 240; y++)
	{
		for(x = 0; x < 320; x++)
		{
			*BITMAP_ADDR32(bitmap, y, x) = (state->screen[x + 320*y].r << 16) | (state->screen[x + 320*y].g << 8) | state->screen[x + 320*y].b;
		}
	}

	return 0;
}

/*************************
*    Machine Hardware    *
*************************/

static void vii_do_dma(running_machine *machine, UINT32 len)
{
	vii_state *state = (vii_state *)machine->driver_data;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 src = state->video_regs[0x70];
	UINT32 dst = state->video_regs[0x71] + 0x2c00;
	UINT32 j;

	for(j = 0; j < len; j++)
	{
		memory_write_word_16le(space, (dst+j) << 1, memory_read_word_16le(space, (src+j) << 1));
	}

	state->video_regs[0x72] = 0;
}

static READ16_HANDLER( vii_video_r )
{
	vii_state *state = (vii_state *)space->machine->driver_data;

	switch(offset)
	{
		case 0x62: // Video IRQ Enable
			verboselog(space->machine, 0, "vii_video_r: Video IRQ Enable: %04x\n", VII_VIDEO_IRQ_ENABLE);
			return VII_VIDEO_IRQ_ENABLE;

		case 0x63: // Video IRQ Status
			verboselog(space->machine, 0, "vii_video_r: Video IRQ Status: %04x\n", VII_VIDEO_IRQ_STATUS);
			return VII_VIDEO_IRQ_STATUS;

		default:
			verboselog(space->machine, 0, "vii_video_r: Unknown register %04x = %04x\n", 0x2800 + offset, state->video_regs[offset]);
			break;
	}
	return state->video_regs[offset];
}

static WRITE16_HANDLER( vii_video_w )
{
	vii_state *state = (vii_state *)space->machine->driver_data;

	switch(offset)
	{
		case 0x62: // Video IRQ Enable
			verboselog(space->machine, 0, "vii_video_w: Video IRQ Enable = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&VII_VIDEO_IRQ_ENABLE);
			break;

		case 0x63: // Video IRQ Acknowledge
			verboselog(space->machine, 0, "vii_video_w: Video IRQ Acknowledge = %04x (%04x)\n", data, mem_mask);
			VII_VIDEO_IRQ_STATUS &= ~data;
			if(!VII_VIDEO_IRQ_STATUS)
			{
				cputag_set_input_line(space->machine, "maincpu", UNSP_IRQ0_LINE, CLEAR_LINE);
			}
			break;

		case 0x70: // Video DMA Source
			verboselog(space->machine, 0, "vii_video_w: Video DMA Source = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->video_regs[offset]);
			break;

		case 0x71: // Video DMA Dest
			verboselog(space->machine, 0, "vii_video_w: Video DMA Dest = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->video_regs[offset]);
			break;

		case 0x72: // Video DMA Length
			verboselog(space->machine, 0, "vii_video_w: Video DMA Length = %04x (%04x)\n", data, mem_mask);
			vii_do_dma(space->machine, data);
			break;

		default:
			verboselog(space->machine, 0, "vii_video_w: Unknown register %04x = %04x (%04x)\n", 0x2800 + offset, data, mem_mask);
			COMBINE_DATA(&state->video_regs[offset]);
			break;
	}
}

static READ16_HANDLER( vii_audio_r )
{
	switch(offset)
	{
		default:
			verboselog(space->machine, 4, "vii_audio_r: Unknown register %04x\n", 0x3000 + offset);
			break;
	}
	return 0;
}

static WRITE16_HANDLER( vii_audio_w )
{
	switch(offset)
	{
		default:
			verboselog(space->machine, 4, "vii_audio_w: Unknown register %04x = %04x (%04x)\n", 0x3000 + offset, data, mem_mask);
			break;
	}
}

static void vii_switch_bank(running_machine *machine, UINT32 bank)
{
	vii_state *state = (vii_state *)machine->driver_data;
	UINT8 *cart = memory_region(machine, "cart");

	if(bank == state->current_bank)
	{
		return;
	}

	state->current_bank = bank;

	memcpy(state->cart, cart + 0x400000 * bank * 2 + 0x4000*2, (0x400000 - 0x4000) * 2);
}

static void vii_do_gpio(running_machine *machine, UINT32 offset)
{
	vii_state *state = (vii_state *)machine->driver_data;
	UINT32 index  = (offset - 1) / 5;
	UINT16 buffer = state->io_regs[5*index + 2];
	UINT16 dir    = state->io_regs[5*index + 3];
	UINT16 attr   = state->io_regs[5*index + 4];

	UINT16 push   = dir;
	UINT16 pull   = (~dir) & (~attr);
	UINT16 what   = (buffer & (push | pull)) ^ (dir &~ attr);

	if(state->spg243_mode == SPG243_VII)
	{
		if(index == 1)
		{
			UINT32 bank = ((what & 0x80) >> 7) | ((what & 0x20) >> 4);
			vii_switch_bank(machine, bank);
		}
	}
	else if(state->spg243_mode == SPG243_BATMAN)
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

	state->io_regs[5*index + 1] = what;
}

static void vii_do_i2c(running_machine *machine)
{
}

static READ16_HANDLER( vii_io_r )
{
	static const char *gpioregs[] = { "GPIO Data Port", "GPIO Buffer Port", "GPIO Direction Port", "GPIO Attribute Port", "GPIO IRQ/Latch Port" };
	static const char gpioports[] = { 'A', 'B', 'C' };

	vii_state *state = (vii_state *)space->machine->driver_data;
	UINT16 val = state->io_regs[offset];

	offset -= 0x500;

	switch(offset)
	{
		case 0x01: case 0x06: case 0x0b: // GPIO Data Port A/B/C
			vii_do_gpio(space->machine, offset);
			verboselog(space->machine, 3, "vii_io_r: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], state->io_regs[offset], mem_mask);
			val = state->io_regs[offset];
			break;

		case 0x02: case 0x03: case 0x04: case 0x05:
		case 0x07: case 0x08: case 0x09: case 0x0a:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // Other GPIO regs
			verboselog(space->machine, 3, "vii_io_r: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], state->io_regs[offset], mem_mask);
			break;

		case 0x1c: // Random
			val = mame_rand(space->machine) & 0x00ff;
			verboselog(space->machine, 3, "vii_io_r: Random = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x22: // IRQ Status
			val = state->io_regs[0x21];
			verboselog(space->machine, 3, "vii_io_r: Controller IRQ Status = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x2c: case 0x2d: // Timers?
			val = mame_rand(space->machine) & 0x0000ffff;
			verboselog(space->machine, 3, "vii_io_r: Unknown Timer %d Register = %04x (%04x)\n", offset - 0x2c, val, mem_mask);
			break;

		case 0x2f: // Data Segment
			val = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), UNSP_SR) >> 10;
			verboselog(space->machine, 3, "vii_io_r: Data Segment = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x31: // Unknown, UART Status?
			verboselog(space->machine, 3, "vii_io_r: Unknown (UART Status?) = %04x (%04x)\n", 3, mem_mask);
			val = 3;
			break;

		case 0x36: // UART RX Data
			val = state->controller_input[state->uart_rx_count];
			state->uart_rx_count = (state->uart_rx_count + 1) % 8;
			verboselog(space->machine, 3, "vii_io_r: UART RX Data = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x59: // I2C Status
			verboselog(space->machine, 3, "vii_io_r: I2C Status = %04x (%04x)\n", val, mem_mask);
			break;

		case 0x5e: // I2C Data In
			verboselog(space->machine, 3, "vii_io_r: I2C Data In = %04x (%04x)\n", val, mem_mask);
			break;

		default:
			verboselog(space->machine, 3, "vii_io_r: Unknown register %04x\n", 0x3800 + offset);
			break;
	}

	return val;
}

static WRITE16_HANDLER( vii_io_w )
{
	static const char *gpioregs[] = { "GPIO Data Port", "GPIO Buffer Port", "GPIO Direction Port", "GPIO Attribute Port", "GPIO IRQ/Latch Port" };
	static const char gpioports[3] = { 'A', 'B', 'C' };

	vii_state *state = (vii_state *)space->machine->driver_data;
	UINT16 temp = 0;

	offset -= 0x500;

	switch(offset)
	{
		case 0x00: // GPIO special function select
			verboselog(space->machine, 3, "vii_io_w: GPIO Function Select = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x01: case 0x06: case 0x0b: // GPIO data, port A/B/C
			offset++;
			// Intentional fallthrough

		case 0x02: case 0x03: case 0x04: case 0x05: // Port A
		case 0x07: case 0x08: case 0x09: case 0x0a: // Port B
		case 0x0c: case 0x0d: case 0x0e: case 0x0f: // Port C
			verboselog(space->machine, 3, "vii_io_w: %s %c = %04x (%04x)\n", gpioregs[(offset - 1) % 5], gpioports[(offset - 1) / 5], data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			vii_do_gpio(space->machine, offset);
			break;

		case 0x21: // IRQ Enable
			verboselog(space->machine, 3, "vii_io_w: Controller IRQ Enable = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&VII_CTLR_IRQ_ENABLE);
			if(!VII_CTLR_IRQ_ENABLE)
			{
				cputag_set_input_line(space->machine, "maincpu", UNSP_IRQ3_LINE, CLEAR_LINE);
			}
			break;

		case 0x22: // IRQ Acknowledge
			verboselog(space->machine, 3, "vii_io_w: Controller IRQ Acknowledge = %04x (%04x)\n", data, mem_mask);
			state->io_regs[0x22] &= ~data;
			if(!state->io_regs[0x22])
			{
				cputag_set_input_line(space->machine, "maincpu", UNSP_IRQ3_LINE, CLEAR_LINE);
			}
			break;

		case 0x2f: // Data Segment
			temp = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), UNSP_SR);
			cpu_set_reg(devtag_get_device(space->machine, "maincpu"), UNSP_SR, (temp & 0x03ff) | ((data & 0x3f) << 10));
			verboselog(space->machine, 3, "vii_io_w: Data Segment = %04x (%04x)\n", data, mem_mask);
			break;

		case 0x31: // Unknown UART
			verboselog(space->machine, 3, "vii_io_w: Unknown UART = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x33: // UART Baud Rate
			verboselog(space->machine, 3, "vii_io_w: UART Baud Rate = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x35: // UART TX Data
			verboselog(space->machine, 3, "vii_io_w: UART TX Data = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5a: // I2C Access Mode
			verboselog(space->machine, 3, "vii_io_w: I2C Access Mode = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5b: // I2C Device Address
			verboselog(space->machine, 3, "vii_io_w: I2C Device Address = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5c: // I2C Sub-Address
			verboselog(space->machine, 3, "vii_io_w: I2C Sub-Address = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5d: // I2C Data Out
			verboselog(space->machine, 3, "vii_io_w: I2C Data Out = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5e: // I2C Data In
			verboselog(space->machine, 3, "vii_io_w: I2C Data In = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x5f: // I2C Controller Mode
			verboselog(space->machine, 3, "vii_io_w: I2C Controller Mode = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;

		case 0x58: // I2C Command
			verboselog(space->machine, 3, "vii_io_w: I2C Command = %04x (%04x)\n", data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			vii_do_i2c(space->machine);
			break;

		case 0x59: // I2C Status / IRQ Acknowledge(?)
			verboselog(space->machine, 3, "vii_io_w: I2C Status / Ack = %04x (%04x)\n", data, mem_mask);
			state->io_regs[offset] &= ~data;
			break;

		default:
			verboselog(space->machine, 3, "vii_io_w: Unknown register %04x = %04x (%04x)\n", 0x3800 + offset, data, mem_mask);
			COMBINE_DATA(&state->io_regs[offset]);
			break;
	}
}

/*
static WRITE16_HANDLER( vii_rowscroll_w )
{
	switch(offset)
	{
		default:
			verboselog(space->machine, 0, "vii_rowscroll_w: %04x = %04x (%04x)\n", 0x2900 + offset, data, mem_mask);
			break;
	}
}

static WRITE16_HANDLER( vii_spriteram_w )
{
	switch(offset)
	{
		default:
			verboselog(space->machine, 0, "vii_spriteram_w: %04x = %04x (%04x)\n", 0x2c00 + offset, data, mem_mask);
			break;
	}
}
*/

static ADDRESS_MAP_START( vii_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x004fff ) AM_RAM AM_BASE_MEMBER(vii_state,ram)
	AM_RANGE( 0x005000, 0x0051ff ) AM_READWRITE(vii_video_r, vii_video_w)
	AM_RANGE( 0x005200, 0x0055ff ) AM_RAM AM_BASE_MEMBER(vii_state,rowscroll)
	AM_RANGE( 0x005600, 0x0057ff ) AM_RAM AM_BASE_MEMBER(vii_state,palette)
	AM_RANGE( 0x005800, 0x005fff ) AM_RAM AM_BASE_MEMBER(vii_state,spriteram)
	AM_RANGE( 0x006000, 0x006fff ) AM_READWRITE(vii_audio_r, vii_audio_w)
	AM_RANGE( 0x007000, 0x007fff ) AM_READWRITE(vii_io_r,    vii_io_w)
	AM_RANGE( 0x008000, 0x7fffff ) AM_ROM AM_BASE_MEMBER(vii_state,cart)
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

static DEVICE_IMAGE_LOAD( vii_cart )
{
	vii_state *state = (vii_state *)image->machine->driver_data;
	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	memcpy(state->cart, cart + 0x4000*2, (0x400000 - 0x4000) * 2);

	if( cart[0x3cd808] == 0x99 &&
		cart[0x3cd809] == 0x99 &&
		cart[0x3cd80a] == 0x83 &&
		cart[0x3cd80b] == 0x5e &&
		cart[0x3cd80c] == 0x52 &&
		cart[0x3cd80d] == 0x6b &&
		cart[0x3cd80e] == 0x78 &&
		cart[0x3cd80f] == 0x7f )
	{
		state->centered_coordinates = 0;
	}
	return INIT_PASS;
}

static MACHINE_START( vii )
{
	vii_state *state = (vii_state *)machine->driver_data;

	memset(state->video_regs, 0, 0x100 * sizeof(UINT16));
	memset(state->io_regs, 0, 0x100 * sizeof(UINT16));
	state->current_bank = 0;

	state->controller_input[0] = 0;
	state->controller_input[4] = 0;
	state->controller_input[6] = 0xff;
	state->controller_input[7] = 0;
}

static MACHINE_RESET( vii )
{
}

static INTERRUPT_GEN( vii_vblank )
{
	vii_state *state = (vii_state *)device->machine->driver_data;
	UINT32 x = mame_rand(device->machine) & 0x3ff;
	UINT32 y = mame_rand(device->machine) & 0x3ff;
	UINT32 z = mame_rand(device->machine) & 0x3ff;

	VII_VIDEO_IRQ_STATUS = VII_VIDEO_IRQ_ENABLE & 1;
	if(VII_VIDEO_IRQ_STATUS)
	{
		verboselog(device->machine, 0, "Video IRQ\n");
		cputag_set_input_line(device->machine, "maincpu", UNSP_IRQ0_LINE, ASSERT_LINE);
	}

	state->controller_input[0] = input_port_read(device->machine, "P1");
	state->controller_input[1] = (UINT8)x;
	state->controller_input[2] = (UINT8)y;
	state->controller_input[3] = (UINT8)z;
	state->controller_input[4] = 0;
	x >>= 8;
	y >>= 8;
	z >>= 8;
	state->controller_input[5] = (z << 4) | (y << 2) | x;
	state->controller_input[6] = 0xff;
	state->controller_input[7] = 0;

	state->uart_rx_count = 0;

	if(VII_CTLR_IRQ_ENABLE)
	{
		verboselog(device->machine, 0, "Controller IRQ\n");
		cputag_set_input_line(device->machine, "maincpu", UNSP_IRQ3_LINE, ASSERT_LINE);
	}
}

static MACHINE_DRIVER_START( vii )

	MDRV_DRIVER_DATA( vii_state )

	MDRV_CPU_ADD( "maincpu", UNSP, XTAL_27MHz)
	MDRV_CPU_PROGRAM_MAP( vii_mem )
	MDRV_CPU_VBLANK_INT("screen", vii_vblank)

	MDRV_MACHINE_START( vii )
	MDRV_MACHINE_RESET( vii )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 240-1)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( vii_cart )

	MDRV_VIDEO_START( vii )
	MDRV_VIDEO_UPDATE( vii )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( batman )

	MDRV_DRIVER_DATA( vii_state )

	MDRV_CPU_ADD( "maincpu", UNSP, XTAL_27MHz)
	MDRV_CPU_PROGRAM_MAP( vii_mem )
	MDRV_CPU_VBLANK_INT("screen", vii_vblank)

	MDRV_MACHINE_START( vii )
	MDRV_MACHINE_RESET( vii )

	MDRV_NVRAM_HANDLER( i2cmem_0 )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 240-1)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START( vii )
	MDRV_VIDEO_UPDATE( vii )
MACHINE_DRIVER_END

static DRIVER_INIT( vii )
{
	vii_state *state = (vii_state *)machine->driver_data;

	state->spg243_mode = SPG243_VII;
	state->centered_coordinates = 1;
}

static DRIVER_INIT( batman )
{
	vii_state *state = (vii_state *)machine->driver_data;

	state->spg243_mode = SPG243_BATMAN;
	state->centered_coordinates = 1;
	i2cmem_init(machine, 0, I2CMEM_SLAVE_ADDRESS, 0, 0x200, NULL);
}

ROM_START( vii )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for u'nSP */

	ROM_REGION( 0x2000000, "cart", ROMREGION_ERASE00 )
ROM_END

ROM_START( batman )
	ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for u'nSP */
	ROM_LOAD16_WORD_SWAP( "batman.bin", 0x000000, 0x400000, CRC(46f848e5) SHA1(5875d57bb3fe0cac5d20e626e4f82a0e5f9bb94c) )
ROM_END

/*    YEAR  NAME     PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY                                              FULLNAME      FLAGS */
CONS( 2004, batman,  0,        0,        batman,   batman,   batman,   "JAKKS Pacific, Inc. / HotGen, Ltd.",                "The Batman", GAME_NO_SOUND )
CONS( 2007, vii,     0,        0,        vii,      vii,      vii,      "Jungle Soft / KenSingTon / Chintendo / Siatronics", "Vii",        GAME_NOT_WORKING | GAME_NO_SOUND)
