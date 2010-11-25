/*****************************************************************************
 *
 * includes/exidy.h
 *
 ****************************************************************************/

#ifndef EXIDY_H_
#define EXIDY_H_

#include "devices/z80bin.h"

typedef struct {
	struct {
		int length;		/* time cassette level is at input.level */
		int level;		/* cassette level */
		int bit;		/* bit being read */
	} input;
	struct {
		int length;		/* time cassette level is at output.level */
		int level;		/* cassette level */
		int bit;		/* bit to to output */
	} output;
} cass_data_t;


class exidy_state : public driver_device
{
public:
	exidy_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	running_device *ay31015;
	UINT8 fe;
	UINT8 keyboard_line;
	emu_timer *serial_timer;
	emu_timer *cassette_timer;
	cass_data_t cass_data;
};


/*----------- defined in machine/exidy.c -----------*/

READ8_HANDLER(exidy_fc_r);
READ8_HANDLER(exidy_fd_r);
READ8_HANDLER(exidy_fe_r);
READ8_HANDLER(exidy_ff_r);
WRITE8_HANDLER(exidy_fc_w);
WRITE8_HANDLER(exidy_fd_w);
WRITE8_HANDLER(exidy_fe_w);
WRITE8_HANDLER(exidy_ff_w);
MACHINE_START( exidy );
MACHINE_RESET( exidy );
Z80BIN_EXECUTE( exidy );
SNAPSHOT_LOAD( exidy );


/*----------- defined in video/exidy.c -----------*/

VIDEO_UPDATE( exidy );


#endif /* EXIDY_H_ */
