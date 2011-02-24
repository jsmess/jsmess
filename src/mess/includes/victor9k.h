#pragma once

#ifndef __VICTOR9K__
#define __VICTOR9K__

#include "machine/ram.h"

#define SCREEN_TAG		"screen"
#define I8088_TAG		"8l"
#define I8048_TAG		"5d"
#define I8022_TAG		"i8022"
#define I8253_TAG		"13h"
#define I8259A_TAG		"7l"
#define UPD7201_TAG		"16e"
#define HD46505S_TAG	"11a"
#define MC6852_TAG		"13b"
#define HC55516_TAG		"1m"
#define M6522_1_TAG		"m6522_1"
#define M6522_2_TAG		"m6522_2"
#define M6522_3_TAG		"m6522_3"
#define M6522_4_TAG		"m6522_4"
#define M6522_5_TAG		"m6522_5"
#define M6522_6_TAG		"m6522_6"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"
#define IEEE488_TAG		"ieee488"

class victor9k_state : public driver_device
{
public:
	victor9k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, I8088_TAG),
		  m_fdc_cpu(*this, I8048_TAG),
		  m_ieee488(*this, IEEE488_TAG),
		  m_pic(*this, I8259A_TAG),
		  m_ssda(*this, MC6852_TAG),
		  m_via1(*this, M6522_1_TAG),
		  m_cvsd(*this, HC55516_TAG),
		  m_crtc(*this, HD46505S_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_fdc_cpu;
	required_device<device_t> m_ieee488;
	required_device<device_t> m_pic;
	required_device<device_t> m_ssda;
	required_device<device_t> m_via1;
	required_device<device_t> m_cvsd;
	required_device<device_t> m_crtc;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE_LINE_MEMBER( vsync_w );
	DECLARE_WRITE_LINE_MEMBER( ssda_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via2_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via3_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via4_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via5_irq_w );
	DECLARE_WRITE_LINE_MEMBER( via6_irq_w );
	DECLARE_WRITE8_MEMBER( via1_pa_w );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( codec_vol_w );
	DECLARE_READ8_MEMBER( via2_pa_r );
	DECLARE_WRITE8_MEMBER( via2_pa_w );
	DECLARE_WRITE8_MEMBER( via2_pb_w );
	DECLARE_READ8_MEMBER( via3_pa_r );
	DECLARE_READ8_MEMBER( via3_pb_r );
	DECLARE_WRITE8_MEMBER( via3_pa_w );
	DECLARE_WRITE8_MEMBER( via3_pb_w );
	DECLARE_WRITE8_MEMBER( via4_pa_w );
	DECLARE_WRITE8_MEMBER( via4_pb_w );
	DECLARE_WRITE_LINE_MEMBER( mode_w );
	DECLARE_READ8_MEMBER( via5_pa_r );
	DECLARE_WRITE8_MEMBER( via5_pb_w );
	DECLARE_READ8_MEMBER( via6_pa_r );
	DECLARE_READ8_MEMBER( via6_pb_r );
	DECLARE_WRITE8_MEMBER( via6_pa_w );
	DECLARE_WRITE8_MEMBER( via6_pb_w );
	DECLARE_WRITE_LINE_MEMBER( drw_w );
	DECLARE_WRITE_LINE_MEMBER( erase_w );

	/* video state */
	UINT8 *m_video_ram;
	int m_vert;
	int m_brt;
	int m_cont;

	/* interrupts */
	int m_via1_irq;
	int m_via2_irq;
	int m_via3_irq;
	int m_via4_irq;
	int m_via5_irq;
	int m_via6_irq;
	int m_ssda_irq;

	/* floppy state */
	int m_lms[2];						/* motor speed */
	int m_st[2];						/* stepper phase */
	int m_se[2];						/* stepper enable */
	int m_drive;						/* selected drive */
	int m_side;							/* selected side */
};

#endif
