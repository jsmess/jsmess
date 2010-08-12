/***************************************************************************


    Funtech Super A'Can
    -------------------

    Preliminary driver by Angelo Salese
    Improvements by Harmony


*******************************************************************************

INFO:

    The system unit contains a reset button.

    Controllers:
    - 4 directional buttons
    - A, B, X, Y, buttons
    - Start, select buttons
    - L, R shoulder buttons

STATUS:

    The driver is begging for a re-write or at least a split into video/supracan.c.  It will happen eventually.

    Sound CPU comms and sound chip are completely unknown.

    There are 6 interrupt sources on the 6502 side, all of which use the IRQ line.
    The register at 0x411 is bitmapped to indicate what source(s) are active.
    In priority order from most to least important, they are:

    411 value  How acked                     Notes
    0x40       read reg 0x16 of sound chip   likely timer. snd regs 0x16/0x17 are time constant, write 0 to reg 0x9f to start
    0x04       read at 0x405                 latch 1?  0xcd is magic value
    0x08       read at 0x404                 latch 2?  0xcd is magic value
    0x10       read at 0x409                 unknown, dispatched but not used in startup 6502 code
    0x20       read at 0x40a                 possible periodic like vblank?
    0x80       read reg 0x14 of sound chip   depends on reg 0x14 of sound chip & 0x40: if not set writes 0x8f to reg 0x14,
                                             otherwise writes 0x4f to reg 0x14 and performs additional processing

    Known unemulated graphical effects and issues:
    - All: Sprite sizing is still imperfect.
    - All: Sprites need to be converted to use scanline rendering for proper clipping.
    - All: Improperly-emulated 1bpp ROZ mode, used by the Super A'Can BIOS logo.
    - All: Unimplemented ROZ scaling tables, used by the Super A'Can BIOS logo and Speedy Dragon intro, among others.
    - All: Priorities are largely unknown.
    - C.U.G.: Gameplay backgrounds are broken.
    - Sango Fighter: Possible missing masking on the upper edges of the screen during gameplay.
    - Speedy Dragon: Backgrounds are broken (wrong tile bank/region).
    - Super Taiwanese Baseball League: Does not boot for unknown reasons.
    - Super Taiwanese Baseball League: Missing window effect applied on tilemaps?
    - The Son of Evil: Does not boot for unknown reasons.
    - The Son of Evil: Many graphical issues.

DEBUG TRICKS:

    baseball game debug trick:
    wpset e90020,1f,w
    do pc=5ac40
    ...
    do pc=5acd4
    wpclear
    bp 0269E4
    [ff7be4] <- 0x269ec
    bpclear

***************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "debugger.h"

#define SOUNDCPU_BOOT_HACK      (1)

#define DRAW_DEBUG_ROZ          (0)

#define DRAW_DEBUG_UNK_SPRITE   (0)

#define DEBUG_PRIORITY          (0)
#define DEBUG_PRIORITY_INDEX    (0) // 0-3

#define VERBOSE_LEVEL   (3)

#define ENABLE_VERBOSE_LOG (1)

typedef struct _acan_dma_regs_t acan_dma_regs_t;
struct _acan_dma_regs_t
{
	UINT32 source[2];
	UINT32 dest[2];
	UINT16 count[2];
	UINT16 control[2];
};

typedef struct _acan_sprdma_regs_t acan_sprdma_regs_t;
struct _acan_sprdma_regs_t
{
	UINT32 src;
    UINT16 src_inc;
	UINT32 dst;
    UINT16 dst_inc;
	UINT16 count;
	UINT16 control;
};

class supracan_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, supracan_state(machine)); }

	supracan_state(running_machine &machine) : driver_data_t(machine)
    {
        m6502_reset = 0;
    }

	acan_dma_regs_t acan_dma_regs;
	acan_sprdma_regs_t acan_sprdma_regs;

	UINT16 m6502_reset;
	UINT8 *soundram;
    UINT8 soundlatch;
    UINT8 soundcpu_irq_src;
    UINT8 sound_irq_enable_reg;
    UINT8 sound_irq_source_reg;
    UINT8 sound_cpu_68k_irq_reg;

	emu_timer *video_timer;
    emu_timer *hbl_timer;
    emu_timer *line_on_timer;
    emu_timer *line_off_timer;
	UINT16 *vram;
    UINT8 bg_color;

    bitmap_t *sprite_final_bitmap;
    UINT8 sprite_pri[1024*1024];
    UINT16 sprite_count;
	UINT32 sprite_base_addr;
    UINT8 sprite_flags;

    bitmap_t *tilemap_bitmap[3];
    bitmap_t *tilemap_final_bitmap[3];
    UINT8 tilemap_pri[3][1024*1024];
    UINT8 tilemap_final_pri[3][320*240];
	UINT32 tilemap_base_addr[3];
	int tilemap_scrollx[3];
    int tilemap_scrolly[3];
	UINT16 video_flags;
	UINT16 tilemap_flags[3];
    UINT16 tilemap_mode[3];
	UINT16 irq_mask;
    UINT16 hbl_mask;

    bitmap_t *roz_bitmap;
    bitmap_t *roz_final_bitmap;
    UINT8 roz_pri[1024*1024];
    UINT8 roz_final_pri[320*240];
	UINT32 roz_base_addr;
	UINT16 roz_mode;
	UINT32 roz_scrollx;
    UINT32 roz_scrolly;
	UINT16 roz_tile_bank;
    UINT32 roz_unk_base0;
    UINT32 roz_unk_base1;
    UINT32 roz_unk_base2;
    UINT16 roz_coeffa;
    UINT16 roz_coeffb;
    UINT16 roz_coeffc;
    UINT16 roz_coeffd;
    INT32 roz_changed;
    INT32 roz_cx;
    INT32 roz_cy;

	UINT16 video_regs[256];

    bool hack_68k_to_6502_access;
};

static READ16_HANDLER( supracan_68k_soundram_r );
static WRITE16_HANDLER( supracan_68k_soundram_w );
static READ16_HANDLER( supracan_sound_r );
static WRITE16_HANDLER( supracan_sound_w );
static READ16_HANDLER( supracan_video_r );
static WRITE16_HANDLER( supracan_video_w );

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(const char *tag, running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%06x: %s: %s", cpu_get_pc(machine->device(tag)), tag, buf );
	}
}

#else
#define verboselog(w,x,y,z,...)
#endif

static VIDEO_START( supracan )
{
    // crash debug: printf("A"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    state->roz_bitmap = auto_bitmap_alloc(machine, 1024, 1024, BITMAP_FORMAT_INDEXED16);
    state->roz_final_bitmap = auto_bitmap_alloc(machine, 320, 240, BITMAP_FORMAT_INDEXED16);
    state->tilemap_bitmap[0] = auto_bitmap_alloc(machine, 1024, 1024, BITMAP_FORMAT_INDEXED16);
    state->tilemap_bitmap[1] = auto_bitmap_alloc(machine, 1024, 1024, BITMAP_FORMAT_INDEXED16);
    state->tilemap_bitmap[2] = auto_bitmap_alloc(machine, 1024, 1024, BITMAP_FORMAT_INDEXED16);
    state->tilemap_final_bitmap[0] = auto_bitmap_alloc(machine, 320, 240, BITMAP_FORMAT_INDEXED16);
    state->tilemap_final_bitmap[1] = auto_bitmap_alloc(machine, 320, 240, BITMAP_FORMAT_INDEXED16);
    state->tilemap_final_bitmap[2] = auto_bitmap_alloc(machine, 320, 240, BITMAP_FORMAT_INDEXED16);
    state->sprite_final_bitmap = auto_bitmap_alloc(machine, 1024, 1024, BITMAP_FORMAT_INDEXED16);
}

static UINT16 fetch_8bpp_tile_pixel(running_machine *machine, int tile, int palette, bool xflip, bool yflip, int x, int y)
{
    // crash debug: printf("B"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;

    if(xflip) x = 7 - x;
    if(yflip) y = 7 - y;

    UINT32 address = (tile * 8 * 8 + y * 8 + x) >> 1;
    UINT8 shift = (x & 1) ? 0 : 8;

    return (supracan_vram[address] >> shift) & 0x00ff;
}

static UINT16 fetch_4bpp_tile_pixel(running_machine *machine, int tile, int palette, bool xflip, bool yflip, int x, int y)
{
    // crash debug: printf("C"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;

    if(xflip) x = 7 - x;
    if(yflip) y = 7 - y;

    UINT32 address = (tile * 8 * 4 + y * 4 + (x >> 1)) >> 1;
    UINT8 shift = (3 - (x & 3)) * 4;

    return (palette << 4) | ((supracan_vram[address] >> shift) & 0x000f);
}

static UINT16 fetch_2bpp_tile_pixel(running_machine *machine, int tile, int palette, bool xflip, bool yflip, int x, int y)
{
    // crash debug: printf("D"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;

    if(xflip) x = 7 - x;
    if(yflip) y = 7 - y;

    UINT32 address = (tile * 8 * 2 + y * 2 + (x >> 2)) >> 1;
    UINT8 shift = (7 - (x & 7)) * 2;

    return (palette << 2) | ((supracan_vram[address] >> shift) & 0x0003);
}

static void draw_tile(running_machine *machine, UINT8* primap, bitmap_t *bitmap, const rectangle *cliprect, int tile, int palette, bool xflip, bool yflip, int x, int y, int priority, int depth, int mask)
{
    // crash debug: printf("E"); fflush(stdout);
    for(int tiley = 0; tiley < 8; tiley++)
    {
        for(int tilex = 0; tilex < 8; tilex++)
        {
            UINT16 pixel = 0;
            switch(depth)
            {
                case 0: // 8bpp
                    pixel = fetch_8bpp_tile_pixel(machine, tile, palette, xflip, yflip, tilex, tiley);
                    break;
                case 1: // 4bpp
                    pixel = fetch_4bpp_tile_pixel(machine, tile, palette, xflip, yflip, tilex, tiley);
                    if((pixel & 0x0f) == 0)
                    {
                        pixel = 0;
                    }
                    break;
                case 2: // 2bpp
                    pixel = fetch_2bpp_tile_pixel(machine, tile, palette, xflip, yflip, tilex, tiley);
                    if((pixel & 0x03) == 0)
                    {
                        pixel = 0;
                    }
                    break;
                default:
                    fatalerror("Unsupported bit depth for draw_tile\n");
                    break;
            }

            int total_x = x + tilex;
            int total_y = y + tiley;
            if((cliprect != NULL && total_x >= cliprect->min_x && total_y >= cliprect->min_y && total_x < cliprect->max_x && total_y < cliprect->max_y) || cliprect == NULL)
            {
                if(pixel)
                {
                    switch(mask)
                    {
                        case 0: // Default
                            *BITMAP_ADDR16(bitmap, total_y, total_x) = pixel;
                            primap[total_y*1024 + total_x] = (UINT8)priority;
                            break;
                        case 1: // Mask against previous alpha
                            if(*BITMAP_ADDR16(bitmap, total_y, total_x) & 0x8000)
                            {
                                *BITMAP_ADDR16(bitmap, total_y, total_x) = pixel | 0x8000;
                                primap[total_y*1024 + total_x] = (UINT8)priority;
                            }
                            break;
                        case 2: // Write to alpha
                            *BITMAP_ADDR16(bitmap, total_y, total_x) |= 0x8000;
                            break;
                        case 3: // Clear mask
                            *BITMAP_ADDR16(bitmap, total_y, total_x) &= 0x7fff;
                            break;
                    }
                }
            }
        }
    }
}

static void draw_tilemap_scanline(running_machine *machine, UINT16 *scanline, UINT8 *priline, int ypos, int layer)
{
    supracan_state *state = machine->driver_data<supracan_state>();

    if((!(state->video_flags & 0x80) && layer == 0) ||
       (!(state->video_flags & 0x40) && layer == 1) ||
       (!(state->video_flags & 0x20) && layer == 2))
    {
        for(int x = 0; x < 320; x++)
        {
            priline[x] = 8;
            scanline[x] = 0;
        }
        return;
    }

    int width = 32;
    int height = 32;
    switch(state->tilemap_flags[layer] & 0x0f00)
    {
        case 0x600:
            width = 64;
            break;

        case 0xa00:
            width = 128;
            break;

        case 0xc00:
            width = 64;
            height = 64;
            break;

        default:
            //verboselog("maincpu", machine, 0, "Unsupported tilemap size: %04x\n", state->tilemap_flags[layer] & 0x0f00);
            break;
    }

    int scrollx = state->tilemap_scrollx[layer];
    int scrolly = state->tilemap_scrolly[layer];
    scrollx |= (scrollx & 0x800) ? 0xfffff000 : 0;
    scrolly |= (scrolly & 0x800) ? 0xfffff000 : 0;

    int gfx_mode = (state->tilemap_mode[layer] & 0x7000) >> 12;

    int region = 1;
    UINT16 tile_bank = 0;
    UINT16 palette_bank = 0;
    switch(gfx_mode)
    {
        case 7:
            region = 2;
            tile_bank = 0x1c00;
            palette_bank = 0x00;
            break;

        case 4:
            region = 1;
            tile_bank = 0x800;
            palette_bank = 0x00;
            break;

        case 2:
            region = 1;
            tile_bank = 0x400;
            palette_bank = 0x00;
            break;

        case 0:
            region = 1;
            tile_bank = 0;
            palette_bank = 0x00;
            break;

        default:
            verboselog("maincpu", machine, 0, "Unsupported tilemap mode: %d\n", (state->tilemap_flags[layer] & 0x7000) >> 12);
            break;
    }

    int wrapped_x = scrollx;
    int wrapped_y = ypos + scrolly;

    if(state->tilemap_flags[layer] & 0x20)
    {
        while(wrapped_y >= height*8) wrapped_y -= height*8;
        while(wrapped_y < 0) wrapped_y += height*8;
    }
    else
    {
        if(wrapped_y < 0 || wrapped_y >= height*8)
        {
            for(int x = 0; x < 320; x++)
            {
                scanline[x] = 0;
                priline[x] = 8;
            }
            return;
        }
    }

    UINT16 *src = BITMAP_ADDR16(state->tilemap_bitmap[layer], wrapped_y, 0);
    UINT8 *srcpri = &state->tilemap_pri[layer][1024*wrapped_y];

    if(state->tilemap_flags[layer] & 0x20)
    {
        while(wrapped_x >= width*8) wrapped_x -= width*8;
        while(wrapped_x < 0) wrapped_x += width*8;

        for(int x = 0; x < 320; x++)
        {
            scanline[x] = src[wrapped_x] ? src[wrapped_x] : scanline[x];
            priline[x] = src[wrapped_x] ? srcpri[wrapped_x] : 8;
            wrapped_x++;
            while(wrapped_x >= width*8) wrapped_x -= width*8;
        }
    }
    else
    {
        for(int x = 0; x < 320; x++)
        {
            if(wrapped_x >= 0 && wrapped_x < width*8)
            {
                scanline[x] = src[wrapped_x] ? src[wrapped_x] : scanline[x];
                priline[x] = src[wrapped_x] ? srcpri[wrapped_x] : 8;
            }
            else
            {
                scanline[x] = scanline[x];
                priline[x] = 8;
            }
            wrapped_x++;
        }
    }
}

static void draw_tilemap(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int layer)
{
    /*crash debug: printf("F"); fflush(stdout); */
    supracan_state *state = machine->driver_data<supracan_state>();
	UINT16 *supracan_vram = state->vram;

	UINT32 count = (state->tilemap_base_addr[layer]);

    int xsize = 32;
    int ysize = 32;
	switch(state->tilemap_flags[layer] & 0x0f00)
	{
		case 0x600:
            xsize = 64;
            ysize = 32;
            break;

        case 0xa00:
            xsize = 128;
            ysize = 32;
            break;

        case 0xc00:
            xsize = 64;
            ysize = 64;
            break;

        default:
            verboselog("maincpu", machine, 0, "Unsupported tilemap size for layer %d: %04x\n", layer, state->tilemap_flags[layer] & 0x0f00);
            break;
	}

    UINT8 priority = ((state->tilemap_flags[layer] >> 12) & 7) + 1;

    int scrollx = state->tilemap_scrollx[layer];
    int scrolly = state->tilemap_scrolly[layer];
    scrollx |= (scrollx & 0x800) ? 0xfffff000 : 0;
    scrolly |= (scrolly & 0x800) ? 0xfffff000 : 0;

	int gfx_mode = (state->tilemap_mode[layer] & 0x7000) >> 12;

    int region = 1;
    UINT16 tile_bank = 0;
    UINT16 palette_bank = 0;
	switch(gfx_mode)
	{
		case 7:
            region = 2;
            tile_bank = 0x1c00;
            palette_bank = 0x00;
            break;

        case 4:
            region = 1;
            tile_bank = 0x800;
            palette_bank = 0x00;
            break;

        case 2:
            region = 1;
            tile_bank = 0x400;
            palette_bank = 0x00;
            break;

        case 0:
            region = 1;
            tile_bank = 0;
            palette_bank = 0x00;
            break;

        default:
            verboselog("maincpu", machine, 0, "Unsupported tilemap mode: %d\n", (state->tilemap_flags[layer] & 0x7000) >> 12);
            break;
	}

    if(state->tilemap_flags[layer] & 0x20)
    {
        bitmap_fill(state->tilemap_bitmap[layer], NULL, 0);

        for(int y = 0; y < ysize; y++)
        {
            for(int x = 0; x < xsize; x++)
            {
                int tile = (supracan_vram[count] & 0x03ff) + tile_bank;
                int flipx = (supracan_vram[count] & 0x0800) ? 1 : 0;
                int flipy = (supracan_vram[count] & 0x0400) ? 1 : 0;
                int palette = ((supracan_vram[count] & 0xf000) >> 12) + palette_bank;

                draw_tile(machine, state->tilemap_pri[layer], bitmap, NULL, tile, palette, flipx, flipy, x*8, y*8, priority, region, 0);

                count++;
            }
        }
    }
    else
    {
        bitmap_fill(state->tilemap_bitmap[layer], NULL, 0);

        for(int y = 0; y < ysize; y++)
        {
            for(int x = 0; x < xsize; x++)
            {
                int tile = (supracan_vram[count] & 0x03ff) + tile_bank;
                int flipx = (supracan_vram[count] & 0x0800) ? 1 : 0;
                int flipy = (supracan_vram[count] & 0x0400) ? 1 : 0;
                int palette = ((supracan_vram[count] & 0xf000) >> 12) + palette_bank;

                draw_tile(machine, state->tilemap_pri[layer], bitmap, cliprect, tile, palette, flipx, flipy, x*8/*-scrollx*/, y*8/*-scrolly*/, priority, region, 0);

                count++;
            }
        }
    }
}

static void draw_roz_bitmap_scanline(running_machine *machine, bitmap_t *roz_bitmap, UINT16 *scanline, UINT8 *priline, int ypos, INT32 X, INT32 Y, INT32 PA, INT32 PB, INT32 PC, INT32 PD, INT32 *currentx, INT32 *currenty, int changed)
{
    // crash debug: printf("G"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();

    INT32 sx = 0;
    INT32 sy = 0;

    switch(state->roz_mode & 0x0f00)
    {
        case 0x600: sx = 64*8; sy = 32*8; break;
        case 0xa00: sx = 128*8; sy = 32*8; break;
        case 0xc00: sx = 64*8; sy = 64*8; break;
        default: sx = 32*8; sy = 32*8; break;
    }

    if((state->roz_mode & 3) == 0)
    {
        sx = 64;
        sy = 32;
    }

    if (X & 0x08000000) X |= 0xf0000000;
    if (Y & 0x08000000) Y |= 0xf0000000;
    if (PA & 0x8000) PA |= 0xffff0000;
    if (PB & 0x8000) PB |= 0xffff0000;
    if (PC & 0x8000) PC |= 0xffff0000;
    if (PD & 0x8000) PD |= 0xffff0000;

    // re-assign parameters for convenience's sake
    INT32 dx = PA;
    INT32 dmx = PB;
    INT32 dy = PC;
    INT32 dmy = PD;
    INT32 startx = X;
    INT32 starty = Y;

    if(ypos == 0)
    {
        changed = 3;
    }

    if(changed & 1)
    {
        *currentx = startx;
    }
    else
    {
        *currentx += dmx;
    }

    if(changed & 2)
    {
        *currenty = starty;
    }
    else
    {
        *currenty += dmy;
    }

    INT32 rx = *currentx;
    INT32 ry = *currenty;

    INT32 pixx = rx >> 8;
    INT32 pixy = ry >> 8;

    for(int x = 0; x < 320; x++)
    {
        if(state->roz_mode & 0x20)
        {
            while(pixx < 0) pixx += sx;
            while(pixy < 0) pixy += sy;
            while(pixx >= sx) pixx -= sx;
            while(pixy >= sy) pixy -= sy;

            scanline[x] = *BITMAP_ADDR16(roz_bitmap, pixy, pixx);
            priline[x] = state->roz_pri[pixy*1024 + pixx];
        }
        else
        {
            if(pixx >= 0 && pixy >= 0 && pixx < sx && pixy < sy)
            {
                scanline[x] = *BITMAP_ADDR16(roz_bitmap, pixy, pixx);
                priline[x] = state->roz_pri[pixy*1024 + pixx];
            }
        }

        rx += dx;
        ry += dy;

        pixx = rx >> 8;
        pixy = ry >> 8;
    }

    state->roz_changed = 0;
}

static void draw_roz_bitmap(running_machine *machine, const rectangle *cliprect)
{
    // crash debug: printf("H"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;
    UINT32 addr = state->roz_tile_bank << 1;

    const int ysize = 32;
    bitmap_t *roz_bitmap = state->roz_bitmap;
    for (int y = 0; y < ysize; y++)
    {
        UINT32 plane_word_msw[2];
        UINT32 plane_word_lsw[2];
        plane_word_msw[0] = ((UINT32)supracan_vram[addr + (y * 8) + 0] << 16) | supracan_vram[addr + (y * 8) + 1];
        plane_word_lsw[0] = ((UINT32)supracan_vram[addr + (y * 8) + 2] << 16) | supracan_vram[addr + (y * 8) + 3];
        plane_word_msw[1] = ((UINT32)supracan_vram[addr + (y * 8) + 4] << 16) | supracan_vram[addr + (y * 8) + 5];
        plane_word_lsw[1] = ((UINT32)supracan_vram[addr + (y * 8) + 6] << 16) | supracan_vram[addr + (y * 8) + 7];
        for (int x = 0; x < 32; x++)
        {
            UINT8 bit = BIT(plane_word_msw[0], 31-x);// | BIT(plane_word_msw[1], 31-x);
            if(bit)
            {
                *BITMAP_ADDR16(roz_bitmap, y, x) = bit;
                state->roz_pri[y*1024 + x] = (state->roz_mode >> 12) & 7;
            }
        }
        for (int x = 0; x < 32; x++)
        {
            UINT8 bit = BIT(plane_word_lsw[0], 31-x);// | BIT(plane_word_lsw[1], 31-x)
            if(bit)
            {
                *BITMAP_ADDR16(roz_bitmap, y, x+32) = bit;
                state->roz_pri[y*1024 + x + 32] = (state->roz_mode >> 12) & 7;
            }
        }
    }
}

static void draw_roz(running_machine *machine, const rectangle *cliprect)
{
    // crash debug: printf("I"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;
    UINT32 roz_base_addr = state->roz_base_addr;
    int region = 0;
    UINT16 tile_bank = 0;

    switch(state->roz_mode & 3) //FIXME: fix gfx bpp order
    {
        case 0:
            return draw_roz_bitmap(machine, cliprect);

        case 1:
            region = 2;
            tile_bank = (state->roz_tile_bank & 0xf000) >> 3;
            break;

        case 2:
            region = 1;
            tile_bank = (state->roz_tile_bank & 0xf000) >> 3;
            break;

        case 3:
            region = 0;
            tile_bank = (state->roz_tile_bank & 0xf000) >> 3;
            break;
    }

    int xsize = 32;
    int ysize = 32;
    switch(state->roz_mode & 0x0f00)
    {
        case 0x600:
            xsize = 64;
            ysize = 32;
            break;

        case 0xa00:
            xsize = 128;
            ysize = 32;
            break;

        case 0xc00:
            xsize = 64;
            ysize = 64;
            break;

        default:
            xsize = 32;
            ysize = 32;
            break;
    }

    UINT32 count = 0;
    UINT8 priority = ((state->roz_mode >> 12) & 7) + 1;

    bitmap_t *roz_bitmap = state->roz_bitmap;
    for (int y = 0; y < ysize; y++)
    {
        for (int x = 0; x < xsize; x++)
        {
            int tile = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x03ff) | tile_bank;
            bool flipx = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x0800) ? true : false;
            bool flipy = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0x0400) ? true : false;
            int palette = (supracan_vram[roz_base_addr | (count & 0xfff)] & 0xf000) >> 12;

            draw_tile(machine, state->roz_pri, roz_bitmap, NULL, tile, palette, flipx, flipy, x*8, y*8, priority, region, 0);

            count++;
        }
    }
}

static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
    // crash debug: printf("J"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
	UINT16 *supracan_vram = state->vram;

#if !(DRAW_DEBUG_UNK_SPRITE)
    int region = (state->sprite_flags & 1) ? 0 : 1; //8bpp : 4bpp
#endif

//      [0]
//      ---h hhh- ---- ---- Y size
//      ---- ---y yyyy yyyy Y offset
//      [1]
//      bbbb ---- ---- ---- sprite offset bank
//      ---- h--- ---- ---- horizontal flip
//      ---- -v-- ---- ---- vertical flip
//      ---- --mm ---- ---- masking
//      ---- ---- ---- -www X size
//      [2]
//      zzz- uuu- ---- ---- unknown
//      ---- ---x xxxx xxxx X offset
//      [3]
//      oooo oooo oooo oooo sprite pointer

    UINT32 start_word = (state->sprite_base_addr >> 1) + 0 * 4;
    UINT32 end_word = start_word + state->sprite_count * 4;
    for(int i = start_word; i < end_word; i += 4)
	{
		int x = supracan_vram[i+2] & 0x01ff;
		int y = supracan_vram[i+0] & 0x01ff;
		int sprite_offset = supracan_vram[i+3] << 2;
		int bank = (supracan_vram[i+1] & 0xf000) >> 12;
        int mask = (supracan_vram[i+1] & 0x0300) >> 8;
		bool hflip = (supracan_vram[i+1] & 0x0800) ? true : false;
		bool vflip = (supracan_vram[i+1] & 0x0400) ? true : false;
#if !(DRAW_DEBUG_UNK_SPRITE)
        UINT8 priority = ((supracan_vram[i+0] >> 12) & 7) + 1;
#endif
		while(x > cliprect->max_x)
        {
			x -= 0x200;
        }

		while(y > cliprect->max_y)
        {
			y -= 0x200;
        }

		if(supracan_vram[i+3] != 0)
		{
            int xsize = 1 << (supracan_vram[i+1] & 7);
            int ysize = ((supracan_vram[i+0] & 0x1e00) >> 9)+1;
            //int ysize = (((supracan_vram[i+0] & 0x0600) >> 9) + 1) * (1 << ((supracan_vram[i+0] & 0x1800) >> 11));
            //if(supracan_vram[i+2] & 0x2000) ysize = xsize;
            //int ysize = ((supracan_vram[i+2] & 0x1e00) >> 9)+1;

			if(vflip)
			{
				for(int ytile = (ysize - 1); ytile >= 0; ytile--)
				{
					if(hflip)
					{
						for(int xtile = (xsize - 1); xtile >= 0; xtile--)
						{
                            UINT16 data = supracan_vram[sprite_offset/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
                            bool xflip = (data & 0x0800) ? false : true;
                            bool yflip = (data & 0x0400) ? false : true;
                            int palette = (data & 0xf000) >> 12;

#if !(DRAW_DEBUG_UNK_SPRITE)
                            draw_tile(machine, state->sprite_pri, bitmap, cliprect, tile, palette, xflip, yflip, x+xtile*8, y+ytile*8, priority, region, mask);
#else
                            tile = 0;
                            xflip = false;
                            yflip = false;
                            palette = 0;
                            for(int ypix = 0; ypix < 8; ypix++)
                            {
                                for(int xpix = 0; xpix < 8; xpix++)
                                {
                                    if((y + ytile*8 + ypix) < 240 &&
                                       (y + ytile*8 + ypix) >= 0 &&
                                       (x + xtile*8 + xpix) < 320 &&
                                       (x + xtile*8 + xpix) >= 0)
                                    {
                                        *BITMAP_ADDR16(bitmap, y + ytile*8 + ypix, x + xtile*8 + xpix) = supracan_vram[i+2] >> 9;
                                    }
                                }
                            }
#endif
						}
					}
					else
					{
						for(int xtile = 0; xtile < xsize; xtile++)
						{
                            UINT16 data = supracan_vram[sprite_offset/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
                            bool xflip = (data & 0x0800) ? true : false;
                            bool yflip = (data & 0x0400) ? false : true;
                            int palette = (data & 0xf000) >> 12;

#if !(DRAW_DEBUG_UNK_SPRITE)
                            draw_tile(machine, state->sprite_pri, bitmap, cliprect, tile, palette, xflip, yflip, x+xtile*8, y+((ysize - 1) - ytile)*8, priority, region, mask);
#else
                            tile = 0;
                            xflip = false;
                            yflip = false;
                            palette = 0;
                            for(int ypix = 0; ypix < 8; ypix++)
                            {
                                for(int xpix = 0; xpix < 8; xpix++)
                                {
                                    if((y + ytile*8 + ypix) < 240 &&
                                       (y + ytile*8 + ypix) >= 0 &&
                                       (x + xtile*8 + xpix) < 320 &&
                                       (x + xtile*8 + xpix) >= 0)
                                    {
                                        *BITMAP_ADDR16(bitmap, y + ytile*8 + ypix, x + xtile*8 + xpix) = supracan_vram[i+2] >> 9;
                                    }
                                }
                            }
#endif
						}
					}
				}
			}
			else
			{
				for(int ytile = 0; ytile < ysize; ytile++)
				{
					if(hflip)
					{
						for(int xtile = (xsize - 1); xtile >= 0; xtile--)
						{
                            UINT16 data = supracan_vram[sprite_offset/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
                            bool xflip = (data & 0x0800) ? false : true;
                            bool yflip = (data & 0x0400) ? true : false;
                            int palette = (data & 0xf000) >> 12;

#if !(DRAW_DEBUG_UNK_SPRITE)
                            draw_tile(machine, state->sprite_pri, bitmap, cliprect, tile, palette, xflip, yflip, x+((xsize - 1) - xtile)*8, y+ytile*8, priority, region, mask);
#else
                            tile = 0;
                            xflip = false;
                            yflip = false;
                            palette = 0;
                            for(int ypix = 0; ypix < 8; ypix++)
                            {
                                for(int xpix = 0; xpix < 8; xpix++)
                                {
                                    if((y + ytile*8 + ypix) < 240 &&
                                       (y + ytile*8 + ypix) >= 0 &&
                                       (x + xtile*8 + xpix) < 320 &&
                                       (x + xtile*8 + xpix) >= 0)
                                    {
                                        *BITMAP_ADDR16(bitmap, y + ytile*8 + ypix, x + xtile*8 + xpix) = supracan_vram[i+2] >> 9;
                                    }
                                }
                            }
#endif
						}
					}
					else
					{
						for(int xtile = 0; xtile < xsize; xtile++)
						{
                            UINT16 data = supracan_vram[sprite_offset/2+ytile*xsize+xtile];
							int tile = (bank * 0x200) + (data & 0x03ff);
                            bool xflip = (data & 0x0800) ? true : false;
                            bool yflip = (data & 0x0400) ? true : false;
                            int palette = (data & 0xf000) >> 12;

#if !(DRAW_DEBUG_UNK_SPRITE)
                            draw_tile(machine, state->sprite_pri, bitmap, cliprect, tile, palette, xflip, yflip, x + xtile * 8, y + ytile * 8, priority, region, mask);
#else
                            tile = 0;
                            xflip = false;
                            yflip = false;
                            palette = 0;
                            for(int ypix = 0; ypix < 8; ypix++)
                            {
                                for(int xpix = 0; xpix < 8; xpix++)
                                {
                                    if((y + ytile*8 + ypix) < 240 &&
                                       (y + ytile*8 + ypix) >= 0 &&
                                       (x + xtile*8 + xpix) < 320 &&
                                       (x + xtile*8 + xpix) >= 0)
                                    {
                                        *BITMAP_ADDR16(bitmap, y + ytile*8 + ypix, x + xtile*8 + xpix) = supracan_vram[i+2] >> 9;
                                    }
                                }
                            }
#endif
						}
					}
				}
			}
		}
	}
}

#if DRAW_DEBUG_ROZ
static void draw_debug_roz(running_machine *machine, bitmap_t *bitmap)
{
    supracan_state *state = machine->driver_data<supracan_state>();
    UINT16 *supracan_vram = state->vram;

    for(int y = 0; y < 240; y++)
    {
        float x = ((float)supracan_vram[state->roz_unk_base0 + y] / 1024.0f) * 256.0f;
        *BITMAP_ADDR16(bitmap, y, (int)x) = 1;

        INT32 temp = supracan_vram[state->roz_unk_base1 + y*2] << 16;
        temp |= supracan_vram[state->roz_unk_base1 + y*2 + 1];
        x = ((float)temp / 65536.0f) * 128.0f;
        temp = supracan_vram[state->roz_unk_base2 + y*2] << 16;
        temp |= supracan_vram[state->roz_unk_base2 + y*2 + 1];
        float y = ((float)temp / 65536.0f) * 128.0f;
        if(x >= -160 && x < 160 && y >= -112 && y < 112)
        {
            *BITMAP_ADDR16(bitmap, (int)y + 112, (int)x + 160) = 2;
        }
    }
}
#endif

static VIDEO_UPDATE( supracan )
{
    // crash debug: printf("K"); fflush(stdout);
    supracan_state *state = (supracan_state *)screen->machine->driver_data<supracan_state>();

    bitmap_fill(bitmap, cliprect, state->bg_color);

    bitmap_fill(state->sprite_final_bitmap, cliprect, 0);
    memset(state->sprite_pri, 8, 1024*1024 * sizeof(UINT8));

    // VIDEO FLAGS                  ROZ MODE            TILEMAP FLAGS
    //
    //  Bit                         Bit                 Bit
    // 15-9: Unknown                15-13: Priority?    15-13: Priority?
    //    8: X ht. (256/320)        12: Unknown         12: Unknown
    //    7: Tilemap 0 enable       11-8: Dims          11-8: Dims
    //    6: Tilemap 1 enable       7-6: Unknown        7-6: Unknown
    //    5: Tilemap 2 enable?      5: Wrap             5: Wrap
    //    3: Sprite enable          4-2: Unknown        4-2: Unknown
    //    2: ROZ enable             1-0: Bit Depth      1-0: Bit Depth
    //  1-0: Unknown

    //                      Video Flags                 ROZ Mode                    Tilemap 0   Tilemap 1   Tilemap 2   VF Unk0
    // A'Can logo:          120e: 0001 0010 0000 1110   4020: 0100 0000 0010 0000   4620        ----        ----        0x09
    // Boomzoo Intro:       9a82: 1001 1010 1000 0010   0402: 0000 0100 0000 0010   6400        6400        4400        0x4d
    // Boomzoo Title:       9acc: 1001 1010 1100 1100   0402: 0000 0100 0000 0010   6400        6400        4400        0x4d
    // C.U.G. Intro:        11c8: 0001 0001 1100 1000   0402: 0000 0100 0000 0010   2400        4400        6400        0x08
    // C.U.G. Title:        11cc: 0001 0001 1100 1100   0602: 0000 0110 0000 0010   2600        4600        ----        0x08
    // Speedy Dragon Logo:  0388: 0000 0011 1000 1000   4020: 0100 0000 0010 0000   6c20        6c20        2600        0x01
    // Speedy Dragon Title: 038c: 0000 0011 1000 1100   2603: 0010 0110 0000 0011   6c20        2c20        2600        0x01
    // Sango Fighter Intro: 03c8: 0000 0011 1100 1000   ----: ---- ---- ---- ----   6c20        4620        ----        0x01
    // Sango Fighter Game:  03ce: 0000 0011 1100 1110   0622: 0000 0110 0010 0010   2620        4620        ----        0x01

    if(state->video_flags & 0x08)
    {
        draw_sprites(screen->machine, state->sprite_final_bitmap, cliprect);
    }

    for(int y = 0; y < 240; y++)
    {
        for(int x = 0; x < 320; x++)
        {

#if !(DEBUG_PRIORITY)
            /* Harmony 7/27: THIS CODE IS KNOWN TO BE COMPLETELY WRONG.  DO NOT USE IT AS-IS IN A STANDALONE EMULATOR. */
            int pri_order[3] = { 0, 0, 0 };
            bitmap_t *bitmaps[3] = { state->tilemap_final_bitmap[0], state->tilemap_final_bitmap[1], state->tilemap_final_bitmap[2] };

            pri_order[0] = state->tilemap_final_pri[0][y*320 + x];
            pri_order[1] = state->tilemap_final_pri[1][y*320 + x];
            pri_order[2] = state->tilemap_final_pri[2][y*320 + x];

            int current_priority = 8;
            for(int index = 0; index < 3; index++)
            {
                UINT16 pixel = *BITMAP_ADDR16(bitmaps[index], y, x) & 0x7fff;
                if(pixel)
                {
                    if(pri_order[index] < current_priority )
                    {
                        *BITMAP_ADDR16(bitmap, y, x) = pixel;
                        current_priority = pri_order[index];
                    }
                }
            }

            UINT16 pixel = *BITMAP_ADDR16(state->roz_final_bitmap, y, x) & 0x7fff;
            if(pixel)
            {
                *BITMAP_ADDR16(bitmap, y, x) = pixel;
            }

            pixel = *BITMAP_ADDR16(state->sprite_final_bitmap, y, x) & 0x7fff;
            if(pixel)
            {
                *BITMAP_ADDR16(bitmap, y, x) = pixel;
            }
#else
            switch(0)
            {
                case 0:
                    *BITMAP_ADDR16(bitmap, y, x) = state->tilemap_pri[0][1024*y + x] + 2;
                    break;
                case 1:
                    *BITMAP_ADDR16(bitmap, y, x) = state->tilemap_pri[1][1024*y + x] + 2;
                    break;
                case 2:
                    *BITMAP_ADDR16(bitmap, y, x) = state->sprite_pri[1024*y + x] + 2;
                    break;
                case 3:
                    *BITMAP_ADDR16(bitmap, y, x) = state->roz_pri[1024*y + x] + 2;
                    break;
            }
#endif
        }
    }

#if DRAW_DEBUG_ROZ
    draw_debug_roz(screen->machine,bitmap);
#endif

	return 0;
}


static WRITE16_HANDLER( supracan_dma_w )
{
    // crash debug: printf("L"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();
	acan_dma_regs_t *acan_dma_regs = &state->acan_dma_regs;
    int ch = (offset < 0x10/2) ? 0 : 1;

	switch(offset)
	{
		case 0x00/2: // Source address MSW
		case 0x10/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: source msw %d: %04x\n", ch, data);
			acan_dma_regs->source[ch] &= 0x0000ffff;
			acan_dma_regs->source[ch] |= data << 16;
			break;
		case 0x02/2: // Source address LSW
		case 0x12/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: source lsw %d: %04x\n", ch, data);
			acan_dma_regs->source[ch] &= 0xffff0000;
			acan_dma_regs->source[ch] |= data;
			break;
		case 0x04/2: // Destination address MSW
		case 0x14/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: dest msw %d: %04x\n", ch, data);
			acan_dma_regs->dest[ch] &= 0x0000ffff;
			acan_dma_regs->dest[ch] |= data << 16;
			break;
		case 0x06/2: // Destination address LSW
		case 0x16/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: dest lsw %d: %04x\n", ch, data);
			acan_dma_regs->dest[ch] &= 0xffff0000;
			acan_dma_regs->dest[ch] |= data;
			break;
		case 0x08/2: // Byte count
		case 0x18/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: count %d: %04x\n", ch, data);
			acan_dma_regs->count[ch] = data;
			break;
		case 0x0a/2: // Control
		case 0x1a/2:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: control %d: %04x\n", ch, data);
			if(data & 0x8800)
			{
//              if(data & 0x2000)
//                  acan_dma_regs->source-=2;
				//verboselog("maincpu", space->machine, 0, "supracan_dma_w: Kicking off a DMA from %08x to %08x, %d bytes (%04x)\n", acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1, data);

				for(int i = 0; i <= acan_dma_regs->count[ch]; i++)
				{
					if(data & 0x1000)
					{
						memory_write_word(space, acan_dma_regs->dest[ch], memory_read_word(space, acan_dma_regs->source[ch]));
						acan_dma_regs->dest[ch]+=2;
						acan_dma_regs->source[ch]+=2;
						if(data & 0x0100)
							if((acan_dma_regs->dest[ch] & 0xf) == 0)
								acan_dma_regs->dest[ch]-=0x10;
					}
					else
					{
						memory_write_byte(space, acan_dma_regs->dest[ch], memory_read_byte(space, acan_dma_regs->source[ch]));
						acan_dma_regs->dest[ch]++;
						acan_dma_regs->source[ch]++;
					}
				}
			}
			else if(data != 0x0000) // fake DMA, used by C.U.G.
			{
				verboselog("maincpu", space->machine, 0, "supracan_dma_w: Unknown DMA kickoff value of %04x (other regs %08x, %08x, %d)\n", data, acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1);
				fatalerror("supracan_dma_w: Unknown DMA kickoff value of %04x (other regs %08x, %08x, %d)",data, acan_dma_regs->source[ch], acan_dma_regs->dest[ch], acan_dma_regs->count[ch] + 1);
			}
			break;
        default:
            verboselog("maincpu", space->machine, 0, "supracan_dma_w: Unknown register: %08x = %04x & %04x\n", 0xe90020 + (offset << 1), data, mem_mask);
            break;
	}
}

static ADDRESS_MAP_START( supracan_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE( 0x000000, 0x3fffff ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xe80200, 0xe80201 ) AM_READ_PORT("P1")
	AM_RANGE( 0xe80202, 0xe80203 ) AM_READ_PORT("P2")
	AM_RANGE( 0xe80208, 0xe80209 ) AM_READ_PORT("P3")
	AM_RANGE( 0xe8020c, 0xe8020d ) AM_READ_PORT("P4")
    AM_RANGE( 0xe80000, 0xe8ffff ) AM_READWRITE( supracan_68k_soundram_r, supracan_68k_soundram_w )
	AM_RANGE( 0xe90000, 0xe9001f ) AM_READWRITE( supracan_sound_r, supracan_sound_w )
	AM_RANGE( 0xe90020, 0xe9003f ) AM_WRITE( supracan_dma_w )
	AM_RANGE( 0xf00000, 0xf001ff ) AM_READWRITE( supracan_video_r, supracan_video_w )
    AM_RANGE( 0xf00200, 0xf003ff ) AM_RAM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE_GENERIC(paletteram)
	AM_RANGE( 0xf40000, 0xf5ffff ) AM_RAM AM_BASE_MEMBER(supracan_state,vram)
	AM_RANGE( 0xfc0000, 0xfdffff ) AM_MIRROR(0x30000) AM_RAM /* System work ram */
ADDRESS_MAP_END

static READ8_HANDLER( supracan_6502_soundmem_r )
{
    // crash debug: printf("M"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();
    UINT8 data = state->soundram[offset];

    switch(offset)
    {
#if SOUNDCPU_BOOT_HACK
        case 0x300: // HACK to make games think the sound CPU is always reporting 'OK'.
            return 0xff;
#endif

        case 0x410: // Sound IRQ enable
            data = state->sound_irq_enable_reg;
            if(!space->debugger_access) verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_r: IRQ enable: %04x\n", data);
            if(!space->debugger_access)
            {
                if(state->sound_irq_enable_reg & state->sound_irq_source_reg)
                {
                    cpu_set_input_line(space->machine->device("soundcpu"), 0, ASSERT_LINE);
                }
                else
                {
                    cpu_set_input_line(space->machine->device("soundcpu"), 0, CLEAR_LINE);
                }
            }
            break;
        case 0x411: // Sound IRQ source
            data = state->sound_irq_source_reg;
            state->sound_irq_source_reg = 0;
            if(!space->debugger_access) verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 3, "supracan_soundreg_r: IRQ source: %04x\n", data);
            if(!space->debugger_access)
            {
                cpu_set_input_line(space->machine->device("soundcpu"), 0, CLEAR_LINE);
            }
            break;
        case 0x420:
            if(!space->debugger_access) verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 3, "supracan_soundreg_r: Sound hardware status? (not yet implemented): %02x\n", 0);
            break;
        case 0x422:
            if(!space->debugger_access) verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 3, "supracan_soundreg_r: Sound hardware data? (not yet implemented): %02x\n", 0);
            break;
        case 0x404:
        case 0x405:
        case 0x409:
        case 0x414:
        case 0x416:
            // Intentional fall-through
        default:
            if(offset >= 0x300 && offset < 0x500)
            {
                if(!space->debugger_access) verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_r: Unknown register %04x\n", offset);
            }
            break;
    }

    return data;
}

static WRITE8_HANDLER( supracan_6502_soundmem_w )
{
    // crash debug: printf("N"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();

    switch(offset)
    {
        case 0x407:
            if(state->sound_cpu_68k_irq_reg &~ data)
            {
                verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_w: sound_cpu_68k_irq_reg: %04x: Triggering M68k IRQ\n", data);
                cpu_set_input_line(space->machine->device("maincpu"), 7, HOLD_LINE);
            }
            else
            {
                verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_w: sound_cpu_68k_irq_reg: %04x\n", data);
            }
            state->sound_cpu_68k_irq_reg = data;
            break;
        case 0x410:
            state->sound_irq_enable_reg = data;
            verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_w: IRQ enable: %02x\n", data);
            break;
        case 0x420:
            verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 3, "supracan_soundreg_w: Sound hardware reg data? (not yet implemented): %02x\n", data);
            break;
        case 0x422:
            verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 3, "supracan_soundreg_w: Sound hardware reg addr? (not yet implemented): %02x\n", data);
            break;
        default:
            if(offset >= 0x300 && offset < 0x500)
            {
                verboselog(state->hack_68k_to_6502_access ? "maincpu" : "soundcpu", space->machine, 0, "supracan_soundreg_w: Unknown register %04x = %02x\n", offset, data);
            }
            state->soundram[offset] = data;
            break;
    }
}

static ADDRESS_MAP_START( supracan_sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE( 0x0000, 0xffff ) AM_READWRITE(supracan_6502_soundmem_r, supracan_6502_soundmem_w) AM_BASE_MEMBER(supracan_state, soundram)
ADDRESS_MAP_END

static INPUT_PORTS_START( supracan )
	PORT_START("P1")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_NAME("P1 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Button A")

	PORT_START("P2")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_NAME("P2 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Button A")

	PORT_START("P3")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(3) PORT_NAME("P3 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(3) PORT_NAME("P3 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(3) PORT_NAME("P3 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3) PORT_NAME("P3 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3) PORT_NAME("P3 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3) PORT_NAME("P3 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3) PORT_NAME("P3 Button A")

	PORT_START("P4")
	PORT_DIPNAME( 0x01, 0x00, "SYSTEM" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(4) PORT_NAME("P4 Button R")
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(4) PORT_NAME("P4 Button L")
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(4) PORT_NAME("P4 Button Y")
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(4) PORT_NAME("P4 Button X")
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Right")
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Left")
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Down")
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4) PORT_NAME("P4 Joypad Up")
	PORT_DIPNAME( 0x1000, 0x0000, "SYSTEM" )
	PORT_DIPSETTING(    0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( On ) )
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(4) PORT_NAME("P4 Button B")
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(4) PORT_NAME("P4 Button A")
INPUT_PORTS_END


static PALETTE_INIT( supracan )
{
	// Used for debugging purposes for now
	//#if 0
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1f) << 3;
		g = ((i >> 5) & 0x1f) << 3;
		b = ((i >> 10) & 0x1f) << 3;
		palette_set_color_rgb( machine, i, r, g, b );
	}
	//#endif
}


static WRITE16_HANDLER( supracan_68k_soundram_w )
{
    // crash debug: printf("P"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();

    state->soundram[offset*2 + 1] = data & 0xff;
    state->soundram[offset*2 + 0] = data >> 8;

    if(offset*2 < 0x500 && offset*2 >= 0x300)
    {
        if(mem_mask & 0xff00)
        {
            state->hack_68k_to_6502_access = true;
            supracan_6502_soundmem_w(space, offset*2, data >> 8);
            state->hack_68k_to_6502_access = false;
        }
        if(mem_mask & 0x00ff)
        {
            state->hack_68k_to_6502_access = true;
            supracan_6502_soundmem_w(space, offset*2 + 1, data & 0xff);
            state->hack_68k_to_6502_access = false;
        }
    }
}

static READ16_HANDLER( supracan_68k_soundram_r )
{
    // crash debug: printf("Q"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();

    UINT16 val = state->soundram[offset*2 + 0] << 8;
    val |= state->soundram[offset*2 + 1];

    if(offset*2 >= 0x300 && offset*2 < 0x500)
    {
        val = 0;
        if(mem_mask & 0xff00)
        {
            state->hack_68k_to_6502_access = true;
            val |= supracan_6502_soundmem_r(space, offset*2) << 8;
            state->hack_68k_to_6502_access = false;
        }
        if(mem_mask & 0x00ff)
        {
            state->hack_68k_to_6502_access = true;
            val |= supracan_6502_soundmem_r(space, offset*2 + 1);
            state->hack_68k_to_6502_access = false;
        }
    }

    return val;
}

static READ16_HANDLER( supracan_sound_r )
{
    // crash debug: printf("R"); fflush(stdout);
    //supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();
    UINT16 data = 0;

    switch( offset )
    {
        default:
            verboselog("maincpu", space->machine, 0, "supracan_sound_r: Unknown register: (%08x) & %04x\n", 0xe90000 + (offset << 1), mem_mask);
            break;
    }

    return data;
}

static WRITE16_HANDLER( supracan_sound_w )
{
    // crash debug: printf("S"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();

	switch ( offset )
	{
		case 0x001c/2:	/* Sound cpu control. Bit 0 tied to sound cpu RESET line */
			if(data & 0x01)
			{
				if(!state->m6502_reset)
				{
					/* Reset and enable the sound cpu */
                    #if !(SOUNDCPU_BOOT_HACK)
					cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, CLEAR_LINE);
					devtag_get_device(space->machine, "soundcpu")->reset();
                    #endif
				}
                state->m6502_reset = data & 0x01;
			}
			else
			{
				/* Halt the sound cpu */
				cputag_set_input_line(space->machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);
			}
            verboselog("maincpu", space->machine, 0, "sound cpu ctrl: %04x\n", data);
			break;
		default:
            verboselog("maincpu", space->machine, 0, "supracan_sound_w: Unknown register: %08x = %04x & %04x\n", 0xe90000 + (offset << 1), data, mem_mask);
			break;
	}
}


static READ16_HANDLER( supracan_video_r )
{
    // crash debug: printf("T"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();
	UINT16 data = state->video_regs[offset];

	switch(offset)
	{
		case 0x00/2: // Video IRQ flags
            if(!space->debugger_access)
            {
                cpu_set_input_line(space->machine->device("maincpu"), 7, CLEAR_LINE);
            }
			break;
        case 0x02/2: // Current scanline
            break;
		default:
            if(!space->debugger_access) verboselog("maincpu", space->machine, 0, "supracan_video_r: Unknown register: %08x (%04x & %04x)\n", 0xf00000 + (offset << 1), data, mem_mask);
			break;
	}

	return data;
}

static TIMER_CALLBACK( supracan_hbl_callback )
{
    // crash debug: printf("U"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();

    timer_adjust_oneshot(state->hbl_timer, attotime_never, 0);
}

static TIMER_CALLBACK( supracan_line_on_callback )
{
    supracan_state *state = machine->driver_data<supracan_state>();

    cpu_set_input_line(machine->device("maincpu"), 5, HOLD_LINE);

    timer_adjust_oneshot(state->line_on_timer, attotime_never, 0);
}

static TIMER_CALLBACK( supracan_line_off_callback )
{
    supracan_state *state = machine->driver_data<supracan_state>();

    cpu_set_input_line(machine->device("maincpu"), 5, CLEAR_LINE);

    timer_adjust_oneshot(state->line_on_timer, attotime_never, 0);
}

static TIMER_CALLBACK( supracan_video_callback )
{
    // crash debug: printf("V"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();
	int vpos = machine->primary_screen->vpos();

    state->video_regs[0] &= ~0x0002;

	switch( vpos )
	{
	case 0:
		state->video_regs[0] &= 0x7fff;
		break;
	case 240:
		state->video_regs[0] |= 0x8000;
        if(state->irq_mask & 1)
        {
            verboselog("maincpu", machine, 0, "Triggering VBL IRQ\n");
            cpu_set_input_line(machine->device("maincpu"), 7, HOLD_LINE);
        }
		break;
    }

    state->video_regs[1] = machine->primary_screen->vpos();

    if (vpos == 0)
    {
        bitmap_fill(state->roz_final_bitmap, &machine->primary_screen->visible_area(), 0);
        bitmap_fill(state->roz_bitmap, &machine->primary_screen->visible_area(), 0);
        bitmap_fill(state->tilemap_bitmap[0], NULL, 0);
        bitmap_fill(state->tilemap_bitmap[1], NULL, 0);
        bitmap_fill(state->tilemap_bitmap[2], NULL, 0);
        bitmap_fill(state->tilemap_final_bitmap[0], NULL, 0);
        bitmap_fill(state->tilemap_final_bitmap[1], NULL, 0);
        bitmap_fill(state->tilemap_final_bitmap[2], NULL, 0);
        //memset(state->roz_pri, 8, 1024*1024 * sizeof(UINT8) );
        //memset(state->tilemap_pri[0], 8, 1024*1024 * sizeof(UINT8));
        //memset(state->tilemap_pri[1], 8, 1024*1024 * sizeof(UINT8));
        //memset(state->tilemap_pri[2], 8, 1024*1024 * sizeof(UINT8));
        //memset(state->tilemap_final_pri[0], 8, 320*240 * sizeof(UINT8));
        //memset(state->tilemap_final_pri[1], 8, 320*240 * sizeof(UINT8));
        //memset(state->tilemap_final_pri[2], 8, 320*240 * sizeof(UINT8));
        if(state->video_flags & 4)
        {
            draw_roz(machine, &machine->primary_screen->visible_area());
        }

        if(state->video_flags & 0x80)
        {
            draw_tilemap(machine, state->tilemap_bitmap[0], &machine->primary_screen->visible_area(), 0);
        }

        if(state->video_flags & 0x40)
        {
            draw_tilemap(machine, state->tilemap_bitmap[1], &machine->primary_screen->visible_area(), 1);
        }

        if(state->video_flags & 0x20)
        {
            draw_tilemap(machine, state->tilemap_bitmap[2], &machine->primary_screen->visible_area(), 2);
        }
    }

    if (vpos < 240)
    {
        UINT16 *scandata = BITMAP_ADDR16(state->roz_final_bitmap, vpos, 0);
        UINT8 *pridata = &state->roz_final_pri[vpos*320];
        draw_roz_bitmap_scanline(machine, state->roz_bitmap, scandata, pridata, vpos, state->roz_scrollx, state->roz_scrolly, state->roz_coeffa, state->roz_coeffb, state->roz_coeffc, state->roz_coeffd, &state->roz_cx, &state->roz_cy, state->roz_changed);

        for(int index = 0; index < 3; index++)
        {
            if(state->video_flags & (0x80 >> index) )
            {
                scandata = BITMAP_ADDR16(state->tilemap_final_bitmap[index], vpos, 0);
                pridata = &state->tilemap_final_pri[index][vpos*320];
                draw_tilemap_scanline(machine, scandata, pridata, vpos, index);
            }
        }
    }

    timer_adjust_oneshot( state->hbl_timer, machine->primary_screen->time_until_pos( vpos, 320 ), 0 );
    timer_adjust_oneshot( state->video_timer, machine->primary_screen->time_until_pos( ( vpos + 1 ) % 256, 0 ), 0 );
}

static WRITE16_HANDLER( supracan_video_w )
{
    // crash debug: printf("W"); fflush(stdout);
    supracan_state *state = (supracan_state *)space->machine->driver_data<supracan_state>();
	acan_sprdma_regs_t *acan_sprdma_regs = &state->acan_sprdma_regs;
	int i;

	switch(offset)
	{
        case 0x10/2: // Byte count
            verboselog("maincpu", space->machine, 0, "sprite dma word count: %04x\n", data);
            acan_sprdma_regs->count = data;
            break;
		case 0x12/2: // Destination address MSW
			acan_sprdma_regs->dst &= 0x0000ffff;
			acan_sprdma_regs->dst |= data << 16;
            verboselog("maincpu", space->machine, 0, "sprite dma dest msw: %04x\n", data);
			break;
		case 0x14/2: // Destination address LSW
			acan_sprdma_regs->dst &= 0xffff0000;
			acan_sprdma_regs->dst |= data;
            verboselog("maincpu", space->machine, 0, "sprite dma dest lsw: %04x\n", data);
			break;
        case 0x16/2: // Source word increment
            verboselog("maincpu", space->machine, 0, "sprite dma dest word inc: %04x\n", data);
            acan_sprdma_regs->dst_inc = data;
            break;
        case 0x18/2: // Source address MSW
            acan_sprdma_regs->src &= 0x0000ffff;
            acan_sprdma_regs->src |= data << 16;
            verboselog("maincpu", space->machine, 0, "sprite dma src msw: %04x\n", data);
            break;
        case 0x1a/2: // Source address LSW
            verboselog("maincpu", space->machine, 0, "sprite dma src lsw: %04x\n", data);
            acan_sprdma_regs->src &= 0xffff0000;
            acan_sprdma_regs->src |= data;
            break;
        case 0x1c/2: // Source word increment
            verboselog("maincpu", space->machine, 0, "sprite dma src word inc: %04x\n", data);
            acan_sprdma_regs->src_inc = data;
            break;
		case 0x1e/2:
			verboselog("maincpu", space->machine, 0, "supracan_dma_w: Kicking off a DMA from %08x to %08x, %d bytes (%04x)\n", acan_sprdma_regs->src, acan_sprdma_regs->dst, acan_sprdma_regs->count, data);

			/* TODO: what's 0x2000 and 0x4000 for? */
			if(data & 0x8000)
			{
				if(data & 0x2000 || data & 0x4000)
                {
					acan_sprdma_regs->dst |= 0xf40000;
                }

                if(data & 0x2000)
                {
                    //acan_sprdma_regs->count <<= 1;
                }

				for(i = 0; i <= acan_sprdma_regs->count; i++)
				{
					if(data & 0x0100) //dma 0x00 fill (or fixed value?)
					{
						memory_write_word(space, acan_sprdma_regs->dst, 0);
                        acan_sprdma_regs->dst+=2 * acan_sprdma_regs->dst_inc;
						//memset(supracan_vram,0x00,0x020000);
					}
					else
					{
						memory_write_word(space, acan_sprdma_regs->dst, memory_read_word(space, acan_sprdma_regs->src));
                        acan_sprdma_regs->dst+=2 * acan_sprdma_regs->dst_inc;
                        acan_sprdma_regs->src+=2 * acan_sprdma_regs->src_inc;
					}
				}
			}
			else
			{
                verboselog("maincpu", space->machine, 0, "supracan_dma_w: Attempting to kick off a DMA without bit 15 set! (%04x)\n", data);
			}
			break;
		case 0x08/2:
			{
                verboselog("maincpu", space->machine, 3, "video_flags = %04x\n", data);
                state->video_flags = data;

                rectangle visarea = space->machine->primary_screen->visible_area();

				visarea.min_x = visarea.min_y = 0;
				visarea.max_y = 240 - 1;
				visarea.max_x = ((state->video_flags & 0x100) ? 320 : 256) - 1;
				space->machine->primary_screen->configure(348, 256, visarea, space->machine->primary_screen->frame_period().attoseconds);
			}
			break;
        case 0x0a/2:
            {
                verboselog("maincpu", space->machine, 0, "IRQ Trigger? = %04x\n", data);
                if(data & 0x8000)
                {
                    timer_adjust_oneshot(state->line_on_timer, space->machine->primary_screen->time_until_pos(data & 0x00ff, 0), 0);
                }
                else
                {
                    timer_adjust_oneshot(state->line_on_timer, attotime_never, 0);
                }
            }
            break;

        case 0x0c/2:
            {
                verboselog("maincpu", space->machine, 0, "IRQ De-Trigger? = %04x\n", data);
                if(data & 0x8000)
                {
                    timer_adjust_oneshot(state->line_off_timer, space->machine->primary_screen->time_until_pos(data & 0x00ff, 0), 0);
                }
                else
                {
                    timer_adjust_oneshot(state->line_off_timer, attotime_never, 0);
                }
            }
            break;

        /* Sprites */
		case 0x20/2: state->sprite_base_addr = data << 2; verboselog("maincpu", space->machine, 0, "sprite_base_addr = %04x\n", data); break;
        case 0x22/2: state->sprite_count = data+1; verboselog("maincpu", space->machine, 0, "sprite_count = %d\n", data+1); break;
        case 0x26/2: state->sprite_flags = data; verboselog("maincpu", space->machine, 0, "sprite_flags = %04x\n", data); break;

        /* Tilemap 0 */
        case 0x100/2: state->tilemap_flags[0] = data; verboselog("maincpu", space->machine, 3, "tilemap_flags[0] = %04x\n", data); break;
        case 0x104/2: state->tilemap_scrollx[0] = data; verboselog("maincpu", space->machine, 3, "tilemap_scrollx[0] = %04x\n", data); break;
        case 0x106/2: state->tilemap_scrolly[0] = data; verboselog("maincpu", space->machine, 3, "tilemap_scrolly[0] = %04x\n", data); break;
        case 0x108/2: state->tilemap_base_addr[0] = (data) << 1; verboselog("maincpu", space->machine, 3, "tilemap_base_addr[0] = %05x\n", data << 2); break;
        case 0x10a/2: state->tilemap_mode[0] = data; verboselog("maincpu", space->machine, 3, "tilemap_mode[0] = %04x\n", data); break;

        /* Tilemap 1 */
        case 0x120/2: state->tilemap_flags[1] = data; verboselog("maincpu", space->machine, 3, "tilemap_flags[1] = %04x\n", data); break;
        case 0x124/2: state->tilemap_scrollx[1] = data; verboselog("maincpu", space->machine, 3, "tilemap_scrollx[1] = %04x\n", data); break;
        case 0x126/2: state->tilemap_scrolly[1] = data; verboselog("maincpu", space->machine, 3, "tilemap_scrolly[1] = %04x\n", data); break;
        case 0x128/2: state->tilemap_base_addr[1] = (data) << 1; verboselog("maincpu", space->machine, 3, "tilemap_base_addr[1] = %05x\n", data << 2); break;
        case 0x12a/2: state->tilemap_mode[1] = data; verboselog("maincpu", space->machine, 3, "tilemap_mode[1] = %04x\n", data); break;

        /* Tilemap 2? */
        case 0x140/2: state->tilemap_flags[2] = data; verboselog("maincpu", space->machine, 0, "tilemap_flags[2] = %04x\n", data); break;
        case 0x144/2: state->tilemap_scrollx[2] = data; verboselog("maincpu", space->machine, 0, "tilemap_scrollx[2] = %04x\n", data); break;
        case 0x146/2: state->tilemap_scrolly[2] = data; verboselog("maincpu", space->machine, 0, "tilemap_scrolly[2] = %04x\n", data); break;
        case 0x148/2: state->tilemap_base_addr[2] = (data) << 1; verboselog("maincpu", space->machine, 0, "tilemap_base_addr[2] = %05x\n", data << 2); verboselog("maincpu", space->machine, 0, "tilemap_base_addr[2] = %05x\n", data << 1); break;
        case 0x14a/2: state->tilemap_mode[2] = data; verboselog("maincpu", space->machine, 0, "tilemap_mode[2] = %04x\n", data); break;

		/* ROZ */
        case 0x180/2: state->roz_mode = data; verboselog("maincpu", space->machine, 3, "roz_mode = %04x\n", data); break;
        case 0x184/2: state->roz_scrollx = (data << 16) | (state->roz_scrollx & 0xffff); state->roz_changed |= 1; break;
        case 0x186/2: state->roz_scrollx = (data) | (state->roz_scrollx & 0xffff0000); state->roz_changed |= 1; break;
        case 0x188/2: state->roz_scrolly = (data << 16) | (state->roz_scrolly & 0xffff); state->roz_changed |= 2; break;
        case 0x18a/2: state->roz_scrolly = (data) | (state->roz_scrolly & 0xffff0000); state->roz_changed |= 2; break;
        case 0x18c/2: state->roz_coeffa = data; break;
        case 0x18e/2: state->roz_coeffb = data; break;
        case 0x190/2: state->roz_coeffc = data; break;
        case 0x192/2: state->roz_coeffd = data; break;
        case 0x194/2: state->roz_base_addr = (data) << 1; verboselog("maincpu", space->machine, 3, "roz_base_addr = %05x\n", data << 2); break;
        case 0x196/2: state->roz_tile_bank = data; verboselog("maincpu", space->machine, 3, "roz_tile_bank = %04x\n", data); break; //tile bank
        case 0x198/2: state->roz_unk_base0 = data << 1; verboselog("maincpu", space->machine, 3, "roz_unk_base0 = %05x\n", data << 2); break;
        case 0x19a/2: state->roz_unk_base1 = data << 1; verboselog("maincpu", space->machine, 3, "roz_unk_base1 = %05x\n", data << 2); break;
        case 0x19e/2: state->roz_unk_base2 = data << 1; verboselog("maincpu", space->machine, 3, "roz_unk_base2 = %05x\n", data << 2); break;

        case 0x1d0/2: state->bg_color = data & 0x00ff; verboselog("maincpu", space->machine, 3, "bg_color = %02x\n", data & 0x00ff); break;

		case 0x1f0/2: //FIXME: this register is mostly not understood
			state->irq_mask = data;//(data & 8) ? 0 : 1;
            /*
            if(!state->irq_mask && !state->hbl_mask)
            {
                cpu_set_input_line(devtag_get_device(space->machine, "maincpu"), 7, CLEAR_LINE);
            }
            */
            verboselog("maincpu", space->machine, 3, "irq_mask = %04x\n", data);
			break;
		default:
			verboselog("maincpu", space->machine, 0, "supracan_video_w: Unknown register: %08x = %04x & %04x\n", 0xf00000 + (offset << 1), data, mem_mask);
			break;
	}
	state->video_regs[offset] = data;
}


static DEVICE_IMAGE_LOAD( supracan_cart )
{
    // crash debug: printf("X"); fflush(stdout);
    UINT8 *cart = memory_region(image.device().machine, "cart");
	UINT32 size;

    if (image.software_entry() == NULL)
	{
        size = image.length();

		if (size > 0x400000)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size");
            return IMAGE_INIT_FAIL;
		}

		if (image.fread(cart, size) != size)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file");
            return IMAGE_INIT_FAIL;
		}
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(cart, image.get_software_region("rom"), size);
	}

    return IMAGE_INIT_PASS;
}


static MACHINE_START( supracan )
{
    // crash debug: printf("Y"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();

	state->video_timer = timer_alloc( machine, supracan_video_callback, NULL );
    state->hbl_timer = timer_alloc( machine, supracan_hbl_callback, NULL );
    state->line_on_timer = timer_alloc( machine, supracan_line_on_callback, NULL );
    state->line_off_timer = timer_alloc( machine, supracan_line_off_callback, NULL );
}


static MACHINE_RESET( supracan )
{
    // crash debug: printf("Z"); fflush(stdout);
    supracan_state *state = machine->driver_data<supracan_state>();

	cputag_set_input_line(machine, "soundcpu", INPUT_LINE_HALT, ASSERT_LINE);

	timer_adjust_oneshot( state->video_timer, machine->primary_screen->time_until_pos( 0, 0 ), 0 );
	state->irq_mask = 0;
}

/* gfxdecode is retained for reference purposes but not otherwise used by the driver */
#ifdef UNUSED_CODE
static const gfx_layout supracan_gfx8bpp =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

static const gfx_layout supracan_gfx4bpp =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static const gfx_layout supracan_gfx2bpp =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0,1 },
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static GFXDECODE_START( supracan )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx8bpp,   0, 1 )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx4bpp,   0, 0x10 )
	GFXDECODE_ENTRY( "ram_gfx",  0, supracan_gfx2bpp,   0, 0x40 )
GFXDECODE_END
#endif

static INTERRUPT_GEN( supracan_irq )
{
    /*
    supracan_state *state = (supracan_state *)device->machine->driver_data<supracan_state>();

	if(state->irq_mask)
    {
		cpu_set_input_line(device, 7, HOLD_LINE);
    }
    */
}

static INTERRUPT_GEN( supracan_sound_irq )
{
    supracan_state *state = (supracan_state *)device->machine->driver_data<supracan_state>();

    cpu_set_input_line(device, 0, HOLD_LINE);
    state->sound_irq_source_reg |= 0x80;
}

static MACHINE_DRIVER_START( supracan )

	MDRV_DRIVER_DATA( supracan_state )

	MDRV_CPU_ADD( "maincpu", M68000, XTAL_10_738635MHz )		/* Correct frequency unknown */
	MDRV_CPU_PROGRAM_MAP( supracan_mem )
	MDRV_CPU_VBLANK_INT("screen", supracan_irq)

	MDRV_CPU_ADD( "soundcpu", M6502, XTAL_3_579545MHz )		/* TODO: Verfiy actual clock */
	MDRV_CPU_PROGRAM_MAP( supracan_sound_mem )
    MDRV_CPU_VBLANK_INT("screen", supracan_sound_irq)

#if !(SOUNDCPU_BOOT_HACK)
    MDRV_QUANTUM_PERFECT_CPU("maincpu")
    MDRV_QUANTUM_PERFECT_CPU("soundcpu")
#endif

	MDRV_MACHINE_START( supracan )
	MDRV_MACHINE_RESET( supracan )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS(XTAL_10_738635MHz/2, 348, 0, 256, 256, 0, 240 )	/* No idea if this is correct */

	MDRV_PALETTE_LENGTH( 32768 )
	MDRV_PALETTE_INIT( supracan )

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("supracan_cart")
	MDRV_CARTSLOT_LOAD(supracan_cart)

	MDRV_SOFTWARE_LIST_ADD("cart_list","supracan")

	MDRV_VIDEO_START( supracan )
	MDRV_VIDEO_UPDATE( supracan )
MACHINE_DRIVER_END


ROM_START( supracan )
	ROM_REGION( 0x400000, "cart", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "ram_gfx", ROMREGION_ERASEFF )
ROM_END


/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT    COMPANY                  FULLNAME        FLAGS */
CONS( 1995, supracan,   0,      0,      supracan,   supracan, 0,      "Funtech Entertainment", "Super A'Can",  GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )
