#pragma once

#ifndef __ABC1600__
#define __ABC1600__

#define MC68008P8_TAG		"3f"
#define Z8410AB1_0_TAG		"5g"
#define Z8410AB1_1_TAG		"7g"
#define Z8410AB1_2_TAG		"8g"
#define Z8470AB1_TAG		"17b"
#define Z8530B1_TAG			"2a"
#define Z8536B1_TAG			"15b"
#define SAB1797_02P_TAG		"5a"
#define FDC9229BT_TAG		"7a"
#define E050_C16PC_TAG		"13b"
#define NMC9306_TAG			"14c"
#define SY6845E_TAG			"sy6845e"
#define SCREEN_TAG			"screen"

class abc1600_state : public driver_device
{
public:
	abc1600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MC68008P8_TAG),
		  m_dma0(*this, Z8410AB1_0_TAG),
		  m_dma1(*this, Z8410AB1_1_TAG),
		  m_dma2(*this, Z8410AB1_2_TAG),
		  m_dart(*this, Z8470AB1_TAG),
		  m_scc(*this, Z8530B1_TAG),
		  m_cio(*this, Z8536B1_TAG),
		  m_fdc(*this, SAB1797_02P_TAG),
		  m_rtc(*this, E050_C16PC_TAG),
		  m_nvram(*this, NMC9306_TAG),
		  m_crtc(*this, SY6845E_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<z80dma_device> m_dma0;
	required_device<z80dma_device> m_dma1;
	required_device<z80dma_device> m_dma2;
	required_device<z80dart_device> m_dart;
	required_device<device_t> m_scc;
	required_device<device_t> m_cio;
	required_device<device_t> m_fdc;
	required_device<device_t> m_rtc;
	optional_device<device_t> m_nvram; // TODO change to required
	required_device<device_t> m_crtc;

	virtual void machine_start();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	
	DECLARE_WRITE8_MEMBER( fw0_w );
	DECLARE_WRITE8_MEMBER( fw1_w );
	DECLARE_WRITE8_MEMBER( en_spec_contr_reg_w );
	
	DECLARE_READ8_MEMBER( cio_pa_r );
	DECLARE_READ8_MEMBER( cio_pb_r );
	DECLARE_WRITE8_MEMBER( cio_pb_w );
	DECLARE_READ8_MEMBER( cio_pc_r );
	DECLARE_WRITE8_MEMBER( cio_pc_w );

	// video
	UINT8 *m_video_ram;
};

#endif
