/*****************************************************************************
 *
 * includes/tandy2k.h
 *
 ****************************************************************************/

#ifndef __TANDY2K_H__
#define __TANDY2K_H__

typedef struct _tandy2k_state tandy2k_state;
struct _tandy2k_state
{
	UINT8 kben;
	UINT8 extclk;
	UINT8 spkrdata;
	UINT8 keylatch;
	UINT8 rxrdy;
	UINT8 txrdy;
	UINT8 outspkr;
	UINT8 pb_sel;
	
	
	/* devices */
	running_device *speaker;
	running_device *pit;
	running_device *fdc;
	running_device *pic0;
	running_device *pic1;
	running_device *uart;
	running_device *centronics;
};


#define I8272A_TAG "i8272a"
#define I8251A_TAG "i8251a"
#define I8253_TAG  "pit"
#define I8255A_TAG  "pio"
#define I8259A_0_TAG "pic0"
#define I8259A_1_TAG "pic1"
#define CENTRONICS_TAG "centronics"
#define I80186_TAG "maincpu"
#define I8048_TAG  "subcpu"
#define SCREEN_TAG "screen"
#define SPEAKER_TAG "speaker"

#endif /* __TANDY2K_H__ */
