/*****************************************************************************

    includes/micronic.h

*****************************************************************************/

#ifndef __MICRONIC__
#define __MICRONIC__

#include "cpu/z80/z80.h"
#include "video/hd61830.h"
#include "machine/mc146818.h"
#include "machine/ram.h"
#include "sound/beep.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define MC146818_TAG	"mc146818"
#define HD61830_TAG		"hd61830"

class micronic_state : public driver_device
{
public:
	micronic_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_lcdc(*this, HD61830_TAG),
		  m_beep(*this, BEEPER_TAG),
		  m_rtc(*this, MC146818_TAG),
		  m_ram(*this, RAM_TAG)
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<hd61830_device> m_lcdc;
	required_device<device_t> m_beep;
	required_device<mc146818_device> m_rtc;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER( keypad_r );
	DECLARE_READ8_MEMBER( status_flag_r );
	DECLARE_WRITE8_MEMBER( status_flag_w );
	DECLARE_WRITE8_MEMBER( kp_matrix_w );
	DECLARE_WRITE8_MEMBER( beep_w );
	DECLARE_READ8_MEMBER( irq_flag_r );
	DECLARE_WRITE8_MEMBER( port_2c_w );
	DECLARE_WRITE8_MEMBER( bank_select_w );
	DECLARE_WRITE8_MEMBER( lcd_contrast_w );
	DECLARE_WRITE8_MEMBER( rtc_address_w );
	DECLARE_READ8_MEMBER( rtc_data_r );
	DECLARE_WRITE8_MEMBER( rtc_data_w );

	void set_146818_periodic_irq(UINT8 data);

	UINT8 *m_ram_base;
	UINT8 m_banks_num;
	UINT8 m_kp_matrix;
	UINT8 m_lcd_contrast;
	bool m_lcd_backlight;
	UINT8 m_status_flag;

	// rtc-146818
	UINT8 m_rtc_address;
	UINT8 m_periodic_irq;
	UINT8 m_irq_flags;
	emu_timer *m_rtc_periodic_irq;
};

#endif
