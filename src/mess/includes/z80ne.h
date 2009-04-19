/*****************************************************************************
 *
 * includes/z80ne.h
 *
 * Nuova Elettronica Z80NE
 *
 * http://www.z80ne.com/
 *
 ****************************************************************************/

#ifndef Z80NE_H_
#define Z80NE_H_

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define Z80NE_CPU_SPEED_HZ		1920000	/* 1.92 MHz */

#define LX383_KEYS			16
#define LX383_DOWNSAMPLING	16

#define LX385_TAPE_SAMPLE_FREQ 38400


/*----------- defined in video/z80ne.c -----------*/

VIDEO_START(lx388);

READ8_HANDLER(lx383_r);
WRITE8_HANDLER(lx383_w);
READ8_HANDLER(lx385_data_r);
WRITE8_HANDLER(lx385_data_w);
READ8_HANDLER(lx385_ctrl_r);
WRITE8_HANDLER(lx385_ctrl_w);
READ8_HANDLER(lx388_data_r);
READ8_HANDLER(lx388_read_field_sync);

DRIVER_INIT(z80ne);
DRIVER_INIT(z80net);
DRIVER_INIT(z80netb);
MACHINE_RESET(z80ne);
MACHINE_RESET(z80net);
MACHINE_RESET(z80netb);
MACHINE_START(z80ne);
MACHINE_START(z80net);
MACHINE_START(z80netb);

INPUT_CHANGED(z80ne_reset);
INPUT_CHANGED(z80ne_nmi);

#endif /* Z80NE_H_ */
