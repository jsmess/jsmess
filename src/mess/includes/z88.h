#include "driver.h"

#define Z88_NUM_COLOURS 3

#define Z88_SCREEN_WIDTH        856
#define Z88_SCREEN_HEIGHT       64

extern PALETTE_INIT( z88 );
extern VIDEO_UPDATE( z88 );
extern VIDEO_EOF( z88 );

#define Z88_AWAKE	0
#define Z88_SNOOZE	1
#define Z88_COMA	2

#define Z88_SNOOZE_TRIGGER 2

struct blink_hw
{
	int z88_state;
	int pb[4];
	int sbr;

    /* screen */
    int sbf;
    int lores0;
    int lores1;
    int hires0;
    int hires1;

	int com;
	int ints;
	int sta;
	int ack;
	int mem[4];
	
	/* rtc acknowledge */
	/* bit 2 = min */
	/* bit 1 = sec */
	/* bit 0 = tick */
	int tack;
	/* rtc interrupt mask */
	int tmk;
	/* rtc interrupt status */
	int tsta;
	/* real time clock registers */
	/* tim0 = 5ms counter */
	/* tim1 = second counter */
	/* tim2 = minute counter */
	/* tim3 = 256 minute counter */
	/* tim4 = 64k minute counter */
	int tim[5];
};

#define RTC_MIN_INT (1<<2) /* once a minute */
#define RTC_SEC_INT (1<<1) /* once a second */
#define RTC_TICK_INT (1<<0) /* 100 times a second */

/* sta bits */
#define STA_TIME (1<<0)
#define STA_KEY (1<<2)

/* ints bits */
#define INT_TIME (1<<1)
#define INT_GINT (1<<0)
#define INT_KWAIT (1<<7)

#define Z88_SCR_HW_REV  (1<<4)
#define Z88_SCR_HW_HRS  (1<<5)
#define Z88_SCR_HW_UND  (1<<1)
#define Z88_SCR_HW_FLS  (1<<3)
#define Z88_SCR_HW_GRY  (1<<2)
