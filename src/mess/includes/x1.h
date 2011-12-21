/*****************************************************************************
 *
 * includes/x1.h
 *
 ****************************************************************************/

#ifndef X1_H_
#define X1_H_

#include "cpu/z80/z80daisy.h"

// ======================> x1_keyboard_device

class x1_keyboard_device :	public device_t,
						public device_z80daisy_interface
{
public:
	// construction/destruction
	x1_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();
};

typedef struct
{
	UINT8 gfx_bank;
	UINT8 disp_bank;
	UINT8 pcg_mode;
	UINT8 v400_mode;
	UINT8 ank_sel;

	UINT8 pri;
	UINT8 blackclip; // x1 turbo specific
} scrn_reg_t;

typedef struct
{
	UINT8 pal;
	UINT8 gfx_pal;
	UINT8 txt_pal[8];
	UINT8 txt_disp;
} turbo_reg_t;

typedef struct
{
	UINT8 sec, min, hour, day, wday, month, year;
} x1_rtc_t;


class x1_state : public driver_device
{
public:
	x1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_tvram;
	UINT8 *m_avram;
	UINT8 *m_kvram;
	UINT8 m_hres_320;
	UINT8 m_io_switch;
	UINT8 m_io_sys;
	UINT8 m_vsync;
	UINT8 m_vdisp;
	UINT8 m_io_bank_mode;
	UINT8 *m_gfx_bitmap_ram;
	UINT8 m_pcg_reset;
	UINT8 m_sub_obf;
	UINT8 m_ctc_irq_flag;
	scrn_reg_t m_scrn_reg;
	turbo_reg_t m_turbo_reg;
	x1_rtc_t m_rtc;
	emu_timer *m_rtc_timer;
	UINT8 m_pcg_write_addr;
	UINT8 m_sub_cmd;
	UINT8 m_sub_cmd_length;
	UINT8 m_sub_val[8];
	int m_sub_val_ptr;
	int m_key_i;
	UINT8 m_irq_vector;
	UINT8 m_cmt_current_cmd;
	UINT8 m_cmt_test;
	UINT8 m_rom_index[3];
	UINT32 m_kanji_offset;
	UINT8 m_bios_offset;
	UINT8 m_x_b;
	UINT8 m_x_g;
	UINT8 m_x_r;
	UINT16 m_kanji_addr_latch;
	UINT32 m_kanji_addr;
	UINT8 m_kanji_eksel;
	UINT8 m_pcg_reset_occurred;
	UINT32 m_old_key1;
	UINT32 m_old_key2;
	UINT32 m_old_key3;
	UINT32 m_old_key4;
	UINT32 m_old_fkey;
	UINT8 m_key_irq_flag;
	UINT8 m_key_irq_vector;
	UINT32 m_emm_addr;
	UINT8 *m_pal_4096;
	UINT8 m_crtc_vreg[0x100],m_crtc_index;
	UINT8 m_is_turbo;
	UINT8 m_ex_bank;
	UINT8 m_ram_bank;
};

SCREEN_UPDATE( x1 );
READ8_HANDLER( x1_sub_io_r );
WRITE8_HANDLER( x1_sub_io_w );
READ8_HANDLER( x1_rom_r );
WRITE8_HANDLER( x1_rom_w );
WRITE8_HANDLER( x1_rom_bank_0_w );
WRITE8_HANDLER( x1_rom_bank_1_w );
READ8_HANDLER( x1_fdc_r );
WRITE8_HANDLER( x1_fdc_w );
READ8_HANDLER( x1_pcg_r );
WRITE8_HANDLER( x1_pcg_w );
WRITE8_HANDLER( x1_pal_r_w );
WRITE8_HANDLER( x1_pal_g_w );
WRITE8_HANDLER( x1_pal_b_w );
WRITE8_HANDLER( x1_ex_gfxram_w );
WRITE8_HANDLER( x1_scrn_w );
WRITE8_HANDLER( x1_pri_w );
WRITE8_HANDLER( x1_6845_w );
READ8_HANDLER( x1_kanji_r );
WRITE8_HANDLER( x1_kanji_w );
READ8_HANDLER( x1_emm_r );
WRITE8_HANDLER( x1_emm_w );
READ8_HANDLER( x1_mem_r );
WRITE8_HANDLER( x1_mem_w );
READ8_HANDLER( x1_io_r );
WRITE8_HANDLER( x1_io_w );


READ8_HANDLER( x1turbo_pal_r );
READ8_HANDLER( x1turbo_txpal_r );
READ8_HANDLER( x1turbo_txdisp_r );
READ8_HANDLER( x1turbo_gfxpal_r );
WRITE8_HANDLER( x1turbo_pal_w );
WRITE8_HANDLER( x1turbo_txpal_w );
WRITE8_HANDLER( x1turbo_txdisp_w );
WRITE8_HANDLER( x1turbo_gfxpal_w );
WRITE8_HANDLER( x1turbo_blackclip_w );
READ8_HANDLER( x1turbo_mem_r );
WRITE8_HANDLER( x1turbo_mem_w );
READ8_HANDLER( x1turbo_io_r );
WRITE8_HANDLER( x1turbo_io_w );

WRITE8_HANDLER( x1turboz_4096_palette_w );
READ8_HANDLER( x1turboz_blackclip_r );

READ8_DEVICE_HANDLER( x1_porta_r );
READ8_DEVICE_HANDLER( x1_portb_r );
READ8_DEVICE_HANDLER( x1_portc_r );
WRITE8_DEVICE_HANDLER( x1_porta_w );
WRITE8_DEVICE_HANDLER( x1_portb_w );
WRITE8_DEVICE_HANDLER( x1_portc_w );

TIMER_DEVICE_CALLBACK(x1_keyboard_callback);
TIMER_CALLBACK(x1_rtc_increment);

MACHINE_RESET( x1 );
MACHINE_RESET( x1turbo );
MACHINE_START( x1 );
PALETTE_INIT(x1);


/*----------- defined in machine/x1.c -----------*/

extern const device_type X1_KEYBOARD;

#endif /* X1_H_ */
