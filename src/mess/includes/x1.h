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

	UINT8 ply;
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

	UINT8 *videoram;
	UINT8 hres_320;
	UINT8 io_switch;
	UINT8 io_sys;
	UINT8 vsync;
	UINT8 vdisp;
	UINT8 io_bank_mode;
	UINT8 *gfx_bitmap_ram;
	UINT16 pcg_index[3];
	UINT8 pcg_reset;
	UINT16 crtc_start_addr;
	UINT8 tile_height;
	UINT8 sub_obf;
	UINT8 ctc_irq_flag;
	scrn_reg_t scrn_reg;
	turbo_reg_t turbo_reg;
	x1_rtc_t rtc;
	emu_timer *rtc_timer;
	UINT8 pcg_write_addr;
	UINT8 *colorram;
	UINT8 sub_cmd;
	UINT8 sub_cmd_length;
	UINT8 sub_val[8];
	int sub_val_ptr;
	int key_i;
	UINT8 irq_vector;
	UINT8 cmt_current_cmd;
	UINT8 cmt_test;
	UINT8 rom_index[3];
	UINT32 kanji_offset;
	UINT8 bios_offset;
	UINT8 pcg_index_r[3];
	UINT16 used_pcg_addr;
	UINT8 x_b;
	UINT8 x_g;
	UINT8 x_r;
	int addr_latch;
	UINT8 kanji_addr_l;
	UINT8 kanji_addr_h;
	UINT8 kanji_count;
	UINT32 kanji_addr;
	UINT8 pcg_reset_occurred;
	UINT32 old_key1;
	UINT32 old_key2;
	UINT32 old_key3;
	UINT32 old_key4;
	UINT32 old_fkey;
	UINT8 key_irq_flag;
	UINT8 key_irq_vector;
};


/*----------- defined in machine/x1.c -----------*/

extern const device_type X1_KEYBOARD;

#endif /* X1_H_ */
