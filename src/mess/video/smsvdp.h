/*************************************************************************

    smsvdp.h

    Implementation of Sega VDP chip used in Master System and Game Gear

**************************************************************************/

#ifndef __SMSVDP_H__
#define __SMSVDP_H__

#include "devcb.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SMS_X_PIXELS			342		/* 342 pixels */
#define SMS_NTSC_Y_PIXELS			262		/* 262 lines */
#define SMS_PAL_Y_PIXELS			313		/* 313 lines */
#define SMS_LBORDER_START			(1 + 2 + 14 + 8)
#define SMS_LBORDER_X_PIXELS		(0x0d)		/* 13 pixels */
#define SMS_RBORDER_X_PIXELS		(0x0f)		/* 15 pixels */
#define SMS_TBORDER_START			(3 + 13)
#define SMS_NTSC_192_TBORDER_Y_PIXELS	(0x1b)		/* 27 lines */
//#define NTSC_192_BBORDER_Y_PIXELS (0x18)      /* 24 lines */
#define SMS_NTSC_224_TBORDER_Y_PIXELS	(0x0b)		/* 11 lines */
//#define NTSC_224_BBORDER_Y_PIXELS (0x08)      /* 8 lines */
//#define PAL_192_TBORDER_Y_PIXELS  (0x36)      /* 54 lines */
//#define PAL_192_BBORDER_Y_PIXELS  (0x30)      /* 48 lines */
//#define PAL_224_TBORDER_Y_PIXELS  (0x26)      /* 38 lines */
//#define PAL_224_BBORDER_Y_PIXELS  (0x20)      /* 32 lines */
#define SMS_PAL_240_TBORDER_Y_PIXELS	(0x1e)		/* 30 lines */
//#define PAL_240_BBORDER_Y_PIXELS  (0x18)      /* 24 lines */


#define SEGA315_5124_PALETTE_SIZE	(64+16)
#define SEGA315_5378_PALETTE_SIZE	4096

PALETTE_INIT( sega315_5124 );
PALETTE_INIT( sega315_5378 );


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _smsvdp_interface smsvdp_interface;
struct _smsvdp_interface
{
	bool               m_is_pal;             /* false = NTSC, true = PAL */
	const char         *m_screen_tag;
	devcb_write_line   m_int_callback;       /* Interrupt callback function */
	devcb_write_line   m_pause_callback;     /* Pause callback function */
};


extern const device_type SEGA315_5124;		/* SMS1 vdp */
extern const device_type SEGA315_5246;		/* SMS2 vdp */
extern const device_type SEGA315_5378;		/* Gamegear vdp */


class sega315_5124_device : public device_t,
                            public smsvdp_interface,
                            public device_memory_interface
{
public:
	// construction/destruction
	sega315_5124_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	sega315_5124_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, UINT8 cram_size, UINT8 palette_offset);

	DECLARE_READ8_MEMBER( vram_read );
	DECLARE_WRITE8_MEMBER( vram_write );
	DECLARE_READ8_MEMBER( register_read );
	DECLARE_WRITE8_MEMBER( register_write );
	DECLARE_READ8_MEMBER( vcount_read );
	DECLARE_READ8_MEMBER( hcount_latch_read );
	DECLARE_WRITE8_MEMBER( hcount_latch_write );

	/* update the screen */
	void update_video( bitmap_t *bitmap, const rectangle *cliprect );

	int check_brightness( int x, int y );
	virtual void set_gg_sms_mode( bool sms_mode ) { };

protected:
	virtual void set_display_settings();
	virtual void update_palette();
	virtual void draw_scanline( int pixel_offset_x, int pixel_plot_y, int line );
	virtual UINT16 get_name_table_address();
	void process_line_timer();
	void draw_scanline_mode4( int *line_buffer, int *priority_selected, int line );
	void draw_sprites_mode4( int *line_buffer, int *priority_selected, int pixel_plot_y, int line );
	void draw_sprites_tms9918_mode( int *line_buffer, int pixel_plot_y, int line );
	void draw_scanline_mode2( int *line_buffer, int line );
	void draw_scanline_mode0( int *line_buffer, int line );

	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const { return (spacenum == AS_0) ? &m_space_config : NULL; }

	UINT8            m_reg[16];                  /* All the registers */
	UINT8            m_status;                   /* Status register */
	UINT8            m_reg9copy;                 /* Internal copy of register 9 */
	UINT8            m_addrmode;                 /* Type of VDP action */
	UINT16           m_addr;                     /* Contents of internal VDP address register */
	UINT8            m_cram_size;                /* CRAM size */
	UINT8            m_cram_mask;                /* Mask to switch between SMS and GG CRAM sizes */
	int              m_cram_dirty;               /* Have there been any changes to the CRAM area */
	int              m_pending;
	UINT8            m_buffer;
	bool             m_gg_sms_mode;              /* Shrunk SMS screen on GG lcd mode flag */
	int              m_irq_state;                /* The status of the IRQ line of the VDP */
	int              m_vdp_mode;                 /* Current mode of the VDP: 0,1,2,3,4 */
	int              m_y_pixels;                 /* 192, 224, 240 */
	int              m_draw_time;
	UINT8            m_line_counter;
	UINT8            m_hcounter;
	memory_region    *m_CRAM;                    /* Pointer to CRAM */
	const UINT8      *m_frame_timing;
	bitmap_t         *m_tmpbitmap;
	bitmap_t         *m_y1_bitmap;
	UINT8            m_collision_buffer[SMS_X_PIXELS];
	UINT8            m_palette_offset;

	/* line_buffer will be used to hold 5 lines of line data. Line #0 is the regular blitting area.
       Lines #1-#4 will be used as a kind of cache to be used for vertical scaling in the gamegear
       sms compatibility mode. */
	int              *m_line_buffer;
	int              m_current_palette[32];
	devcb_resolved_write_line	m_cb_int;
	devcb_resolved_write_line   m_cb_pause;
	emu_timer        *m_smsvdp_display_timer;
	emu_timer        *m_set_status_vint_timer;
	emu_timer        *m_set_status_sprovr_timer;
	emu_timer        *m_set_status_sprcol_timer;
	emu_timer        *m_check_hint_timer;
	emu_timer        *m_check_vint_timer;
	emu_timer        *m_draw_timer;
	screen_device    *m_screen;

	const address_space_config  m_space_config;

	/* Timers */
	static const device_timer_id TIMER_LINE = 0;
	static const device_timer_id TIMER_SET_STATUS_VINT = 1;
	static const device_timer_id TIMER_SET_STATUS_SPROVR = 2;
	static const device_timer_id TIMER_CHECK_HINT = 3;
	static const device_timer_id TIMER_CHECK_VINT = 4;
	static const device_timer_id TIMER_SET_STATUS_SPRCOL = 5;
	static const device_timer_id TIMER_DRAW = 6;
};


class sega315_5246_device : public sega315_5124_device
{
public:
	sega315_5246_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void set_display_settings();
	virtual UINT16 get_name_table_address();
};


class sega315_5378_device : public sega315_5124_device
{
public:
	sega315_5378_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void device_reset();

	virtual void set_gg_sms_mode( bool sms_mode );

protected:
	virtual void set_display_settings();
	virtual void update_palette();
	virtual void draw_scanline( int pixel_offset_x, int pixel_plot_y, int line );
	virtual UINT16 get_name_table_address();
};


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_SEGA315_5124_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, SEGA315_5124, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#define MCFG_SEGA315_5246_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, SEGA315_5246, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#define MCFG_SEGA315_5378_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, SEGA315_5378, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#endif /* __SMSVDP_H__ */
