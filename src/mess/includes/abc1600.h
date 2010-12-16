#pragma once

#ifndef __ABC1600__
#define __ABC1600__

#define MC68008P8_TAG		"3f"
#define Z8410AB1_0_TAG		"5g"
#define Z8410AB1_1_TAG		"7g"
#define Z8410AB1_2_TAG		"8g"
#define Z8470AB1_TAG		"17b"
#define Z8536B1_TAG			"15b"
#define SAB1797_02P_TAG		"5a"
#define FDC9229BT_TAG		"7a"
#define E050_C16PC_TAG		"13b"
#define SY6845E_TAG			"sy6845e"
#define Z8400A_TAG			"z80"
#define SCREEN_TAG			"screen"

class abc1600_state : public driver_device
{
public:
	abc1600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MC68008P8_TAG),
		  m_crtc(*this, SY6845E_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<running_device> m_crtc;

	virtual void machine_start();

	virtual void video_start();
	virtual bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	// video
	UINT8 *m_video_ram;
};

#endif
