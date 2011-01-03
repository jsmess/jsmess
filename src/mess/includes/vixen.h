#pragma once

#ifndef __VIXEN__
#define __VIXEN__

#define Z8400A_TAG		"5f"
#define FDC1797_TAG		"5n"
#define P8155H_TAG		"2n"
#define DISCRETE_TAG	"discrete"
#define SCREEN_TAG		"screen"

class vixen_state : public driver_device
{
public:
	vixen_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z8400A_TAG),
		  m_fdc(*this, FDC1797_TAG),
		  m_discrete(*this, DISCRETE_TAG),
		  m_ram(*this, "messram"),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_fdc;
	required_device<device_t> m_discrete;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void update_interrupt();

	DECLARE_READ8_MEMBER( status_r );
	DECLARE_WRITE8_MEMBER( cmd_w );
	DECLARE_READ8_MEMBER( i8155_pa_r );
	DECLARE_WRITE8_MEMBER( i8155_pb_w );
	DECLARE_WRITE8_MEMBER( i8155_pc_w );
	DECLARE_WRITE_LINE_MEMBER( fdint_w );

	// memory state
	int m_reset;

	// keyboard state
	UINT8 m_col;

	// interrupt state
	UINT8 m_cmd;
	int m_fdint;
	int m_vsync;

	// video state
	int m_alt;
	int m_256;
	UINT8 *m_video_ram;
	const UINT8 *m_sync_rom;
	const UINT8 *m_char_rom;
};

#endif
