/*****************************************************************************
 *
 * includes/sorcerer.h
 *
 ****************************************************************************/

#ifndef SORCERER_H_
#define SORCERER_H_

#include "cpu/z80/z80.h"
#include "sound/dac.h"
#include "sound/wave.h"
#include "machine/ctronics.h"
#include "machine/ram.h"
#include "imagedev/cassette.h"
#include "imagedev/snapquik.h"
#include "imagedev/cartslot.h"
#include "machine/ay31015.h"
#include "imagedev/flopdrv.h"
#include "formats/sorc_dsk.h"

#define SORCERER_USING_DISKS 0
#define SORCERER_USING_RS232 0

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


class sorcerer_state : public driver_device
{
public:
	sorcerer_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_cass1(*this, "cassette1"),
		  m_cass2(*this, "cassette2"),
		  m_wave1(*this, "wave.1"),
		  m_wave2(*this, "wave.2"),
		  m_dac(*this, "dac"),
		  m_uart(*this, "uart"),
		  m_printer(*this, "centronics"),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cass1;
	required_device<device_t> m_cass2;
	required_device<device_t> m_wave1;
	required_device<device_t> m_wave2;
	required_device<device_t> m_dac;
	required_device<device_t> m_uart;
	required_device<device_t> m_printer;
	required_device<device_t> m_ram;
	DECLARE_READ8_MEMBER(sorcerer_fc_r);
	DECLARE_READ8_MEMBER(sorcerer_fd_r);
	DECLARE_READ8_MEMBER(sorcerer_fe_r);
	DECLARE_READ8_MEMBER(sorcerer_ff_r);
	DECLARE_WRITE8_MEMBER(sorcerer_fc_w);
	DECLARE_WRITE8_MEMBER(sorcerer_fd_w);
	DECLARE_WRITE8_MEMBER(sorcerer_fe_w);
	DECLARE_WRITE8_MEMBER(sorcerer_ff_w);
	UINT8 m_fe;
	UINT8 m_keyboard_line;
	emu_timer *m_serial_timer;
	emu_timer *m_cassette_timer;
	cass_data_t m_cass_data;
};


/*----------- defined in machine/sorcerer.c -----------*/


MACHINE_START( sorcerer );
MACHINE_RESET( sorcerer );
Z80BIN_EXECUTE( sorcerer );
SNAPSHOT_LOAD( sorcerer );

#endif /* SORCERER_H_ */
