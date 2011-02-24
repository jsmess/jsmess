#ifndef __XEROX820__
#define __XEROX820__

#include "machine/ram.h"

#define SCREEN_TAG		"screen"

#define Z80_TAG			"u46"
#define Z80KBPIO_TAG	"u105"
#define Z80GPPIO_TAG	"u101"
#define Z80SIO_TAG		"u96"
#define Z80CTC_TAG		"u99"
#define WD1771_TAG		"u109"
#define COM8116_TAG		"u76"
#define I8086_TAG		"i8086"

#define XEROX820_VIDEORAM_SIZE	0x1000
#define XEROX820_VIDEORAM_MASK	0x0fff

class xerox820_state : public driver_device
{
public:
	xerox820_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_kbpio(*this, Z80KBPIO_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_fdc(*this, WD1771_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_kbpio;
	required_device<device_t> m_ctc;
	required_device<device_t> m_fdc;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	DECLARE_WRITE8_MEMBER( scroll_w );
	//DECLARE_WRITE8_MEMBER( x120_system_w );
	DECLARE_READ8_MEMBER( kbpio_pa_r );
	DECLARE_WRITE8_MEMBER( kbpio_pa_w );
	DECLARE_READ8_MEMBER( kbpio_pb_r );
	DECLARE_WRITE_LINE_MEMBER( intrq_w );
	DECLARE_WRITE_LINE_MEMBER( drq_w );

	void scan_keyboard();
	void bankswitch(int bank);
	void set_floppy_parameters(size_t length);
	void common_kbpio_pa_w(UINT8 data);

	/* keyboard state */
	int m_keydata;						/* keyboard data */

	/* video state */
	UINT8 *m_video_ram;					/* video RAM */
	UINT8 *m_char_rom;					/* character ROM */
	UINT8 m_scroll;						/* vertical scroll */
	UINT8 m_framecnt;
	int m_ncset2;						/* national character set */
	int m_vatt;							/* X120 video attribute */
	int m_lowlite;						/* low light attribute */
	int m_chrom;						/* character ROM index */

	/* floppy state */
	int m_fdc_irq;						/* interrupt request */
	int m_fdc_drq;						/* data request */
	int m_8n5;							/* 5.25" / 8" drive select */
	int m_dsdd;							/* double sided disk detect */
};

class xerox820ii_state : public xerox820_state
{
public:
	xerox820ii_state(running_machine &machine, const driver_device_config_base &config)
		: xerox820_state(machine, config)
	{ }

	virtual void machine_reset();

	DECLARE_WRITE8_MEMBER( bell_w );
	DECLARE_WRITE8_MEMBER( slden_w );
	DECLARE_WRITE8_MEMBER( chrom_w );
	DECLARE_WRITE8_MEMBER( lowlite_w );
	DECLARE_WRITE8_MEMBER( sync_w );
	DECLARE_WRITE8_MEMBER( kbpio_pa_w );

	void bankswitch(int bank);
};

#endif
