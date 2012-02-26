/*****************************************************************************
 *
 * includes/z88.h
 *
 ****************************************************************************/

#ifndef Z88_H_
#define Z88_H_

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/upd65031.h"
#include "sound/speaker.h"
#include "rendlay.h"

#define Z88_NUM_COLOURS 3

#define Z88_SCREEN_WIDTH        856
#define Z88_SCREEN_HEIGHT       64

#define Z88_SCR_HW_REV  (1<<4)
#define Z88_SCR_HW_HRS  (1<<5)
#define Z88_SCR_HW_UND  (1<<1)
#define Z88_SCR_HW_FLS  (1<<3)
#define Z88_SCR_HW_GRY  (1<<2)

class z88_state : public driver_device
{
public:
	z88_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;

	UINT8 m_frame_number;
	bool m_flash_invert;

	virtual void machine_start();
	void bankswitch_update(int bank, UINT16 page, int rams);
	DECLARE_READ8_MEMBER(kb_r);
};


/*----------- defined in video/z88.c -----------*/

extern PALETTE_INIT( z88 );
extern UPD65031_SCREEN_UPDATE(z88_screen_update);
extern SCREEN_VBLANK( z88 );


#endif /* Z88_H_ */
