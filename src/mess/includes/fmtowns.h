#ifndef FMTOWNS_H_
#define FMTOWNS_H_

#include "emu.h"

READ8_HANDLER( towns_gfx_high_r );
WRITE8_HANDLER( towns_gfx_high_w );
READ8_HANDLER( towns_gfx_r );
WRITE8_HANDLER( towns_gfx_w );
READ8_HANDLER( towns_video_cff80_r );
WRITE8_HANDLER( towns_video_cff80_w );
READ8_HANDLER( towns_video_cff80_mem_r );
WRITE8_HANDLER( towns_video_cff80_mem_w );
READ8_HANDLER(towns_video_440_r);
WRITE8_HANDLER(towns_video_440_w);
READ8_HANDLER(towns_video_5c8_r);
WRITE8_HANDLER(towns_video_5c8_w);
READ8_HANDLER(towns_video_fd90_r);
WRITE8_HANDLER(towns_video_fd90_w);
READ8_HANDLER(towns_video_ff81_r);
WRITE8_HANDLER(towns_video_ff81_w);
READ8_HANDLER(towns_spriteram_low_r);
WRITE8_HANDLER(towns_spriteram_low_w);
READ8_HANDLER( towns_spriteram_r);
WRITE8_HANDLER( towns_spriteram_w);

struct towns_cdrom_controller
{
	UINT8 command;
	UINT8 status;
	UINT8 cmd_status[4];
	UINT8 cmd_status_ptr;
	UINT8 extra_status;
	UINT8 parameter[8];
	UINT8 mpu_irq_enable;
	UINT8 dma_irq_enable;
	UINT8 buffer[2048];
	INT32 buffer_ptr;
	UINT32 lba_current;
	UINT32 lba_last;
	UINT32 cdda_current;
	UINT32 cdda_length;
	emu_timer* read_timer;
};

struct towns_video_controller
{
	UINT8 towns_vram_wplane;
	UINT8 towns_vram_rplane;
	UINT8 towns_vram_page_sel;
	UINT8 towns_palette_select;
	UINT8 towns_palette_r[256];
	UINT8 towns_palette_g[256];
	UINT8 towns_palette_b[256];
	UINT8 towns_degipal[8];
	UINT8 towns_dpmd_flag;
	UINT8 towns_crtc_mix;
	UINT8 towns_crtc_sel;  // selected CRTC register
	UINT16 towns_crtc_reg[32];
	UINT8 towns_video_sel;  // selected video register
	UINT8 towns_video_reg[2];
	UINT8 towns_sprite_sel;  // selected sprite register
	UINT8 towns_sprite_reg[8];
	UINT8 towns_tvram_enable;
	UINT16 towns_kanji_offset;
	UINT8 towns_kanji_code_h;
	UINT8 towns_kanji_code_l;
	rectangle towns_crtc_layerscr[2];  // each layer has independent sizes
	UINT8 towns_display_plane;
	UINT8 towns_display_page_sel;
	UINT8 towns_vblank_flag;
	UINT8 towns_layer_ctrl;
};

class towns_state : public driver_data_t
{
	public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, towns_state(machine)); }
	towns_state(running_machine &machine)
		: driver_data_t(machine) { }
		
	UINT8 ftimer;
	UINT8 nmi_mask;
	UINT8 compat_mode;
	UINT8 towns_system_port;
	UINT32 towns_ankcg_enable;
	UINT32 towns_mainmem_enable;
	UINT32 towns_ram_enable;
	UINT8* towns_cmos;
	UINT32* towns_vram;
	UINT8* towns_gfxvram;
	UINT8* towns_txtvram;
	int towns_selected_drive;
	UINT8 towns_fdc_irq6mask;
	UINT8* towns_serial_rom;
	int towns_srom_position;
	UINT8 towns_srom_clk;
	UINT8 towns_srom_reset;
	UINT8 towns_rtc_select;
	UINT8 towns_rtc_data;
	UINT8 towns_rtc_reg[16];
	emu_timer* towns_rtc_timer;
	UINT8 towns_timer_mask;
	UINT16 towns_machine_id;  // default is 0x0101
	UINT8 towns_kb_status;
	UINT8 towns_kb_irq1_enable;
	UINT8 towns_kb_output;  // key output
	UINT8 towns_kb_extend;  // extended key output
	emu_timer* towns_kb_timer;
	emu_timer* towns_mouse_timer;
	UINT8 towns_fm_irq_flag;
	UINT8 towns_pcm_irq_flag;
	UINT8 towns_pcm_channel_flag;
	UINT8 towns_pcm_channel_mask;
	UINT8 towns_pad_mask;
	UINT8 towns_mouse_output;
	UINT8 towns_mouse_x;
	UINT8 towns_mouse_y;
	struct towns_cdrom_controller towns_cd;
	struct towns_video_controller video;

	/* devices */
	running_device* maincpu;
	running_device* dma_1;
	running_device* dma_2;
	running_device* fdc;
	running_device* pic_master;
	running_device* pic_slave;
	running_device* pit;
	running_device* messram;
	running_device* cdrom;
	running_device* cdda;

};

void towns_update_video_banks(address_space*);

INTERRUPT_GEN( towns_vsync_irq );
VIDEO_START( towns );
VIDEO_UPDATE( towns );

#endif /*FMTOWNS_H_*/
