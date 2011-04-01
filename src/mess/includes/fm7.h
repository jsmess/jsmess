/*
 *
 *  FM-7 header file
 *
 */

#ifndef FM7_H_
#define FM7_H_

// Interrupt flags
#define IRQ_FLAG_KEY      0x01
#define IRQ_FLAG_PRINTER  0x02
#define IRQ_FLAG_TIMER    0x04
#define IRQ_FLAG_OTHER    0x08
// the following are not read in port 0xfd03
#define IRQ_FLAG_MFD      0x10
#define IRQ_FLAG_TXRDY    0x20
#define IRQ_FLAG_RXRDY    0x40
#define IRQ_FLAG_SYNDET   0x80

// system types
#define SYS_FM7        1
#define SYS_FM77AV     2
#define SYS_FM77AV40EX 3

// keyboard scancode formats
#define KEY_MODE_FM7   0 // FM-7 ASCII type code
#define KEY_MODE_FM16B 1 // FM-16B (FM-77AV and later only)
#define KEY_MODE_SCAN  2 // Scancode Make/Break (PC-like)

typedef struct
{
	UINT8 buffer[12];
	UINT8 tx_count;
	UINT8 rx_count;
	UINT8 command_length;
	UINT8 answer_length;
	UINT8 latch;  // 0=ready to receive
	UINT8 ack;
	UINT8 position;
} fm7_encoder_t;

typedef struct
{
	UINT8 bank_addr[8][16];
	UINT8 segment;
	UINT8 window_offset;
	UINT8 enabled;
	UINT8 mode;
} fm7_mmr_t;

typedef struct
{
	UINT8 sub_busy;
	UINT8 sub_halt;
	UINT8 sub_reset;  // high if reset caused by subrom change
	UINT8 attn_irq;
	UINT8 vram_access;  // VRAM access flag
	UINT8 crt_enable;
	UINT16 vram_offset;
	UINT16 vram_offset2;
	UINT8 fm7_pal[8];
	UINT16 fm77av_pal_selected;
	UINT8 fm77av_pal_r[4096];
	UINT8 fm77av_pal_g[4096];
	UINT8 fm77av_pal_b[4096];
	UINT8 subrom;  // currently active sub CPU ROM (AV only)
	UINT8 cgrom;  // currently active CGROM (AV only)
	UINT8 modestatus;
	UINT8 multi_page;
	UINT8 fine_offset;
	UINT8 nmi_mask;
	UINT8 active_video_page;
	UINT8 display_video_page;
	UINT8 vsync_flag;
} fm7_video_t;

typedef struct
{
	UINT8 command;
	UINT8 lcolour;
	UINT8 mask;
	UINT8 compare_data;
	UINT8 compare[8];
	UINT8 bank_disable;
	UINT8 tilepaint_b;
	UINT8 tilepaint_r;
	UINT8 tilepaint_g;
	UINT16 addr_offset;
	UINT16 line_style;
	UINT16 x0;
	UINT16 x1;
	UINT16 y0;
	UINT16 y1;
	UINT8 busy;
} fm7_alu_t;


class fm7_state : public driver_device
{
public:
	fm7_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8* m_boot_ram;
	UINT8 m_irq_flags;
	UINT8 m_irq_mask;
	emu_timer* m_timer;
	emu_timer* m_subtimer;
	emu_timer* m_keyboard_timer;
	UINT8 m_basic_rom_en;
	UINT8 m_init_rom_en;
	unsigned int m_key_delay;
	unsigned int m_key_repeat;
	UINT16 m_current_scancode;
	UINT32 m_key_data[4];
	UINT32 m_mod_data;
	UINT8 m_key_scan_mode;
	UINT8 m_break_flag;
	UINT8 m_psg_regsel;
	UINT8 m_psg_data;
	UINT8 m_fdc_side;
	UINT8 m_fdc_drive;
	UINT8 m_fdc_irq_flag;
	UINT8 m_fdc_drq_flag;
	UINT8 m_fm77av_ym_irq;
	UINT8 m_speaker_active;
	UINT16 m_kanji_address;
	fm7_encoder_t m_encoder;
	fm7_mmr_t m_mmr;
	UINT8 m_cp_prev;
	UINT8* m_video_ram;
	UINT8* m_shared_ram;
	emu_timer* m_fm77av_vsync_timer;
	UINT8 m_type;
	fm7_video_t m_video;
	fm7_alu_t m_alu;
	int m_sb_prev;
};


/*----------- defined in drivers/fm7.c -----------*/

READ8_HANDLER( fm7_sub_keyboard_r );
READ8_HANDLER( fm77av_key_encoder_r );
WRITE8_HANDLER( fm77av_key_encoder_w );
READ8_HANDLER( fm7_sub_beeper_r );


/*----------- defined in video/fm7.c -----------*/

TIMER_CALLBACK( fm77av_vsync );

READ8_HANDLER( fm7_subintf_r );
WRITE8_HANDLER( fm7_subintf_w );
READ8_HANDLER( fm7_sub_busyflag_r );
WRITE8_HANDLER( fm7_sub_busyflag_w );
READ8_HANDLER( fm77av_sub_modestatus_r );
WRITE8_HANDLER( fm77av_sub_modestatus_w );
WRITE8_HANDLER( fm77av_sub_bank_w );

READ8_HANDLER( fm7_cancel_ack );
READ8_HANDLER( fm7_attn_irq_r );

READ8_HANDLER( fm7_vram_access_r );
WRITE8_HANDLER( fm7_vram_access_w );
READ8_HANDLER( fm7_vram_r );
WRITE8_HANDLER( fm7_vram_w );
READ8_HANDLER( fm7_crt_r );
WRITE8_HANDLER( fm7_crt_w );
WRITE8_HANDLER( fm7_vram_offset_w );
READ8_HANDLER( fm77av_video_flags_r );
WRITE8_HANDLER( fm77av_video_flags_w );
READ8_HANDLER( fm77av_alu_r );
WRITE8_HANDLER( fm77av_alu_w );

WRITE8_HANDLER( fm77av_analog_palette_w );
WRITE8_HANDLER( fm7_multipage_w );
READ8_HANDLER( fm7_palette_r );
WRITE8_HANDLER( fm7_palette_w );

READ8_HANDLER( fm7_vram0_r );
READ8_HANDLER( fm7_vram1_r );
READ8_HANDLER( fm7_vram2_r );
READ8_HANDLER( fm7_vram3_r );
READ8_HANDLER( fm7_vram4_r );
READ8_HANDLER( fm7_vram5_r );
READ8_HANDLER( fm7_vram6_r );
READ8_HANDLER( fm7_vram7_r );
READ8_HANDLER( fm7_vram8_r );
READ8_HANDLER( fm7_vram9_r );
READ8_HANDLER( fm7_vramA_r );
READ8_HANDLER( fm7_vramB_r );
WRITE8_HANDLER( fm7_vram0_w );
WRITE8_HANDLER( fm7_vram1_w );
WRITE8_HANDLER( fm7_vram2_w );
WRITE8_HANDLER( fm7_vram3_w );
WRITE8_HANDLER( fm7_vram4_w );
WRITE8_HANDLER( fm7_vram5_w );
WRITE8_HANDLER( fm7_vram6_w );
WRITE8_HANDLER( fm7_vram7_w );
WRITE8_HANDLER( fm7_vram8_w );
WRITE8_HANDLER( fm7_vram9_w );
WRITE8_HANDLER( fm7_vramA_w );
WRITE8_HANDLER( fm7_vramB_w );

READ8_HANDLER( fm7_sub_ram_ports_banked_r );
WRITE8_HANDLER( fm7_sub_ram_ports_banked_w );
READ8_HANDLER( fm7_console_ram_banked_r );
WRITE8_HANDLER( fm7_console_ram_banked_w );

VIDEO_START( fm7 );
SCREEN_UPDATE( fm7 );
PALETTE_INIT( fm7 );

#endif /*FM7_H_*/
