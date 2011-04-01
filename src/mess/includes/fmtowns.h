#ifndef FMTOWNS_H_
#define FMTOWNS_H_

#include "emu.h"

#define IRQ_LOG 0  // set to 1 to log IRQ line activity

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
READ8_HANDLER(towns_spriteram_r);
WRITE8_HANDLER(towns_spriteram_w);

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
	UINT8 towns_sprite_flag;  // sprite drawing flag
	UINT8 towns_sprite_page;  // VRAM page (not layer) sprites are drawn to
	UINT8 towns_tvram_enable;
	UINT16 towns_kanji_offset;
	UINT8 towns_kanji_code_h;
	UINT8 towns_kanji_code_l;
	rectangle towns_crtc_layerscr[2];  // each layer has independent sizes
	UINT8 towns_display_plane;
	UINT8 towns_display_page_sel;
	UINT8 towns_vblank_flag;
	UINT8 towns_layer_ctrl;
	emu_timer* sprite_timer;
};

class towns_state : public driver_device
{
	public:
	towns_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_nvram(*this, "nvram") { }

	UINT8 m_ftimer;
	UINT16 m_freerun_timer;
	emu_timer* m_towns_freerun_counter;
	UINT8 m_nmi_mask;
	UINT8 m_compat_mode;
	UINT8 m_towns_system_port;
	UINT32 m_towns_ankcg_enable;
	UINT32 m_towns_mainmem_enable;
	UINT32 m_towns_ram_enable;
	UINT32* m_towns_vram;
	UINT8* m_towns_gfxvram;
	UINT8* m_towns_txtvram;
	int m_towns_selected_drive;
	UINT8 m_towns_fdc_irq6mask;
	UINT8* m_towns_serial_rom;
	int m_towns_srom_position;
	UINT8 m_towns_srom_clk;
	UINT8 m_towns_srom_reset;
	UINT8 m_towns_rtc_select;
	UINT8 m_towns_rtc_data;
	UINT8 m_towns_rtc_reg[16];
	emu_timer* m_towns_rtc_timer;
	UINT8 m_towns_timer_mask;
	UINT16 m_towns_machine_id;  // default is 0x0101
	UINT8 m_towns_kb_status;
	UINT8 m_towns_kb_irq1_enable;
	UINT8 m_towns_kb_output;  // key output
	UINT8 m_towns_kb_extend;  // extended key output
	emu_timer* m_towns_kb_timer;
	emu_timer* m_towns_mouse_timer;
	UINT8 m_towns_fm_irq_flag;
	UINT8 m_towns_pcm_irq_flag;
	UINT8 m_towns_pcm_channel_flag;
	UINT8 m_towns_pcm_channel_mask;
	UINT8 m_towns_pad_mask;
	UINT8 m_towns_mouse_output;
	UINT8 m_towns_mouse_x;
	UINT8 m_towns_mouse_y;
	UINT8 m_towns_volume[8];  // volume ports
	UINT8 m_towns_volume_select;
	UINT8 m_towns_scsi_control;
	UINT8 m_towns_scsi_status;
	UINT8 m_towns_spkrdata;
	UINT8 m_towns_speaker_input;

	emu_timer* m_towns_wait_timer;
	struct towns_cdrom_controller m_towns_cd;
	struct towns_video_controller m_video;
	required_shared_ptr<UINT8>	m_nvram;

	/* devices */
	device_t* m_maincpu;
	device_t* m_dma_1;
	device_t* m_dma_2;
	device_t* m_fdc;
	device_t* m_pic_master;
	device_t* m_pic_slave;
	device_t* m_pit;
	device_t* m_messram;
	device_t* m_cdrom;
	device_t* m_cdda;
	device_t* m_speaker;
	class fmscsi_device* m_scsi;
	device_t* m_hd0;
	device_t* m_hd1;
	device_t* m_hd2;
	device_t* m_hd3;
	device_t* m_hd4;
	device_t* m_ram;
	UINT32 m_kb_prev[4];
	UINT8 m_prev_pad_mask;
	UINT8 m_prev_x;
	UINT8 m_prev_y;
};

void towns_update_video_banks(address_space*);

INTERRUPT_GEN( towns_vsync_irq );
VIDEO_START( towns );
SCREEN_UPDATE( towns );

#endif /*FMTOWNS_H_*/
