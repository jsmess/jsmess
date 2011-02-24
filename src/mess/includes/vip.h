#pragma once

#ifndef __VIP__
#define __VIP__

#include "cpu/cosmac/cosmac.h"
#include "audio/vp550.h"
#include "audio/vp595.h"
#include "machine/ram.h"

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"u1"
#define CDP1861_TAG		"u2"
#define CDP1862_TAG		"cdp1862"
#define CASSETTE_TAG	"cassette"
#define DISCRETE_TAG	"discrete"

#define VP590_COLOR_RAM_SIZE	0x100

enum
{
	VIDEO_CDP1861 = 0,
	VIDEO_CDP1862
};

enum
{
	SOUND_SPEAKER = 0,
	SOUND_VP595,
	SOUND_VP550,
	SOUND_VP551
};

enum
{
	KEYBOARD_KEYPAD = 0,
	KEYBOARD_VP580,
	KEYBOARD_DUAL_VP580,
	KEYBOARD_VP601,
	KEYBOARD_VP611
};

enum
{
	LED_POWER = 0,
	LED_Q,
	LED_TAPE
};

class vip_state : public driver_device
{
public:
	vip_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vdc(*this, CDP1861_TAG),
		  m_cgc(*this, CDP1862_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_beeper(*this, DISCRETE_TAG),
		  m_vp595(*this, VP595_TAG),
		  m_vp550(*this, VP550_TAG),
		  m_vp551(*this, VP551_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cosmac_device> m_maincpu;
	required_device<device_t> m_vdc;
	required_device<device_t> m_cgc;
	required_device<device_t> m_cassette;
	required_device<device_t> m_beeper;
	required_device<device_t> m_vp595;
	required_device<device_t> m_vp550;
	required_device<device_t> m_vp551;
	required_device<device_t> m_ram;

	virtual void machine_start();
	virtual void machine_reset();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_READ8_MEMBER(dispon_r);
	DECLARE_WRITE8_MEMBER(dispoff_w);
	DECLARE_WRITE8_MEMBER(keylatch_w);
	DECLARE_WRITE8_MEMBER(bankswitch_w);
	DECLARE_WRITE8_MEMBER(vip_colorram_w);

	/* video state */
	int m_a12;						/* latched address line 12 */
	UINT8 *m_colorram;				/* CDP1862 color RAM */
	UINT8 m_color;					/* color RAM data */

	/* keyboard state */
	int m_keylatch;					/* key latch */
};

#endif
