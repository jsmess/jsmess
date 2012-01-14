/*****************************************************************************
 *
 * includes/z88.h
 *
 ****************************************************************************/

#ifndef Z88_H_
#define Z88_H_


#define Z88_NUM_COLOURS 3

#define Z88_SCREEN_WIDTH        856
#define Z88_SCREEN_HEIGHT       64

#define RTC_MIN_INT (1<<2) /* once a minute */
#define RTC_SEC_INT (1<<1) /* once a second */
#define RTC_TICK_INT (1<<0) /* 100 times a second */

/* sta bits */
#define STA_TIME (1<<0)
#define STA_KEY (1<<2)

/* ints bits */
#define INT_GINT (1<<0)
#define INT_TIME (1<<1)
#define INT_KEY (1<<2)
#define INT_KWAIT (1<<7)

#define Z88_SCR_HW_REV  (1<<4)
#define Z88_SCR_HW_HRS  (1<<5)
#define Z88_SCR_HW_UND  (1<<1)
#define Z88_SCR_HW_FLS  (1<<3)
#define Z88_SCR_HW_GRY  (1<<2)

#define Z88_AWAKE	0
#define Z88_SNOOZE	1
#define Z88_COMA	2

#define Z88_SNOOZE_TRIGGER 2
#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"


typedef struct
{
	UINT8 z88_state;
	UINT16 pb[4];
	UINT16 sbr;

	/* screen */
	UINT32 sbf;
	UINT32 lores0;
	UINT32 lores1;
	UINT32 hires0;
	UINT32 hires1;

	UINT8 com;
	UINT8 ints;
	UINT8 sta;
	UINT8 ack;
	UINT8 mem[4];

	/* rtc acknowledge */
	/* bit 2 = min */
	/* bit 1 = sec */
	/* bit 0 = tick */
	UINT8 tack;
	/* rtc interrupt mask */
	UINT8 tmk;
	/* rtc interrupt status */
	UINT8 tsta;
	/* real time clock registers */
	/* tim0 = 5ms counter */
	/* tim1 = second counter */
	/* tim2 = minute counter */
	/* tim3 = 256 minute counter */
	/* tim4 = 64k minute counter */
	UINT8 tim[5];
} blink_hw_t;


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
	DECLARE_READ8_MEMBER(z88_port_r);
	DECLARE_WRITE8_MEMBER(z88_port_w);
	UINT8 m_frame_number;
	bool m_flash_invert;
	blink_hw_t m_blink;
	void z88_refresh_memory_bank(UINT8 bank);
	void z88_install_memory_handler_pair(offs_t start, offs_t size, UINT8 bank_base, UINT8 *read_addr, UINT8 *write_addr);
	virtual void machine_reset();
};


/*----------- defined in video/z88.c -----------*/

extern PALETTE_INIT( z88 );
extern SCREEN_UPDATE_IND16( z88 );
extern SCREEN_VBLANK( z88 );


#endif /* Z88_H_ */
