#ifndef _WSWAN_H_
#define _WSWAN_H_

#include "sound/custom.h"

#define WSWAN_TYPE_MONO 0
#define WSWAN_TYPE_COLOR 1

#define WSWAN_X_PIXELS	(28*8)
#define WSWAN_Y_PIXELS	(18*8)

/* Interrupt flags */
#define WSWAN_IFLAG_STX    0x1
#define WSWAN_IFLAG_KEY    0x2
#define WSWAN_IFLAG_RTC    0x4
#define WSWAN_IFLAG_SRX    0x8
#define WSWAN_IFLAG_LCMP   0x10
#define WSWAN_IFLAG_VBLTMR 0x20
#define WSWAN_IFLAG_VBL    0x40
#define WSWAN_IFLAG_HBLTMR 0x80
/* Interrupts */
#define WSWAN_INT_STX    0
#define WSWAN_INT_KEY    1
#define WSWAN_INT_RTC    2
#define WSWAN_INT_SRX    3
#define WSWAN_INT_LCMP   4
#define WSWAN_INT_VBLTMR 5
#define WSWAN_INT_VBL    6
#define WSWAN_INT_HBLTMR 7


struct VDP
{
	UINT8 layer_bg_enable;			/* Background layer on/off */
	UINT8 layer_fg_enable;			/* Foreground layer on/off */
	UINT8 sprites_enable;			/* Sprites on/off */
	UINT8 window_sprites_enable;		/* Sprite window on/off */
	UINT8 window_fg_mode;			/* 0:inside/outside, 1:??, 2:inside, 3:outside */
	UINT8 current_line;			/* Current scanline : 0-158 (159?) */
	UINT8 line_compare;			/* Line to trigger line interrupt on */
	UINT32 sprite_table_address;		/* Address of the sprite table */
	UINT8 sprite_first;			/* First sprite to draw */
	UINT8 sprite_count;			/* Number of sprites to draw */
	UINT16 layer_bg_address;		/* Address of the background screen map */
	UINT16 layer_fg_address;		/* Address of the foreground screen map */
	UINT8 window_fg_left;			/* Left coordinate of foreground window */
	UINT8 window_fg_top;			/* Top coordinate of foreground window */
	UINT8 window_fg_right;			/* Right coordinate of foreground window */
	UINT8 window_fg_bottom;			/* Bottom coordinate of foreground window */
	UINT8 window_sprites_left;		/* Left coordinate of sprites window */
	UINT8 window_sprites_top;		/* Top coordinate of sprites window */
	UINT8 window_sprites_right;		/* Right coordinate of sprites window */
	UINT8 window_sprites_bottom;		/* Bottom coordinate of sprites window */
	UINT8 layer_bg_scroll_x;		/* Background layer X scroll */
	UINT8 layer_bg_scroll_y;		/* Background layer Y scroll */
	UINT8 layer_fg_scroll_x;		/* Foreground layer X scroll */
	UINT8 layer_fg_scroll_y;		/* Foreground layer Y scroll */
	UINT8 lcd_enable;			/* LCD on/off */
	UINT8 icons;				/* FIXME: What do we do with these? Maybe artwork? */
	UINT8 color_mode;			/* monochrome/color mode */
	UINT8 colors_16;			/* 4/16 colors mode */
	UINT8 tile_packed;			/* layered/packed tile mode switch */
	UINT8 timer_hblank_enable;		/* Horizontal blank interrupt on/off */
	UINT8 timer_hblank_mode;		/* Horizontal blank timer mode */
	UINT16 timer_hblank_reload;		/* Horizontal blank timer reload value */
	UINT16 timer_hblank_count;		/* Horizontal blank timer counter value */
	UINT8 timer_vblank_enable;		/* Vertical blank interrupt on/off */
	UINT8 timer_vblank_mode;		/* Vertical blank timer mode */
	UINT16 timer_vblank_reload;		/* Vertical blank timer reload value */
	UINT16 timer_vblank_count;		/* Vertical blank timer counter value */
	UINT8 *vram;				/* pointer to start of ram/vram (set by MACHINE_RESET) */
	UINT8 *palette_vram;			/* pointer to start of palette area in ram/vram (set by MACHINE_RESET), WSC only */
	int main_palette[8];
	UINT8 display_vertical;			/* Should the wonderswan be held vertically? */
	UINT8 new_display_vertical;		/* New value for the display_vertical bit (to prevent mid frame changes) */
};

extern struct VDP vdp;
extern UINT8 ws_portram[256];

extern NVRAM_HANDLER( wswan );
extern MACHINE_START( wswan );
extern MACHINE_START( wscolor );
extern MACHINE_RESET( wswan );
extern READ8_HANDLER( wswan_port_r );
extern WRITE8_HANDLER( wswan_port_w );
extern READ8_HANDLER( wswan_sram_r );
extern WRITE8_HANDLER( wswan_sram_w );
extern DEVICE_INIT(wswan_cart);
extern DEVICE_LOAD(wswan_cart);
extern INTERRUPT_GEN(wswan_scanline_interrupt);

/* vidhrdw/wswan.c */
extern void wswan_refresh_scanline(void);

/* sndhrdw/wswan.c */
extern WRITE8_HANDLER( wswan_sound_port_w );
extern void *wswan_sh_start(int clock, const struct CustomSound_interface *config);

#endif /* _WSWAN_H_ */
