/*****************************************************************************
 *
 * includes/x1.h
 *
 ****************************************************************************/

#ifndef X1_H_
#define X1_H_

#include "cpu/z80/z80daisy.h"

// ======================>  x1_keyboard_device_config

class x1_keyboard_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class x1_keyboard_device;

	// construction/destruction
	x1_keyboard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
};



// ======================> x1_keyboard_device

class x1_keyboard_device :	public device_t,
						public device_z80daisy_interface
{
	friend class x1_keyboard_device_config;

	// construction/destruction
	x1_keyboard_device(running_machine &_machine, const x1_keyboard_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const x1_keyboard_device_config &m_config;
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
	x1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_tvram[0x800];
	UINT8 m_avram[0x800];
	UINT8 m_kvram[0x800];
	UINT8 m_hres_320;
	UINT8 m_io_switch;
	UINT8 m_io_sys;
	UINT8 m_vsync;
	UINT8 m_vdisp;
	UINT8 m_io_bank_mode;
	UINT8 m_gfx_bitmap_ram[0xc000*2];
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
	UINT8 m_pal_4096[0x1000*3];
	UINT8 m_crtc_vreg[0x100],m_crtc_index;
	UINT8 m_is_turbo;
	UINT8 m_ex_bank;
	UINT8 m_ram_bank;
};


/*----------- defined in machine/x1.c -----------*/

extern const device_type X1_KEYBOARD;

#endif /* X1_H_ */
