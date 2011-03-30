#pragma once

#ifndef __PROF80__
#define __PROF80__

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "video/mc6845.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/i8255a.h"
#include "machine/upd765.h"
#include "machine/upd1990a.h"
#include "machine/z80sti.h"
#include "machine/ctronics.h"
#include "machine/rescap.h"
#include "sound/speaker.h"

#define Z80_TAG					"z1"
#define UPD765_TAG				"z38"
#define UPD1990A_TAG			"z43"

/* ------------------------------------------------------------------------ */

#define SCREEN_TAG				"screen"
#define GRIP_Z80_TAG			"grip_z1"
#define MC6845_TAG				"z30"
#define I8255A_TAG				"z6"
#define Z80STI_TAG				"z9"
#define SPEAKER_TAG				"speaker"
#define CENTRONICS_TAG			"centronics"

#define GRIP_VIDEORAM_SIZE	0x10000
#define GRIP_VIDEORAM_MASK	0xffff

/* ------------------------------------------------------------------------ */

#define UNIO_Z80STI_TAG			"z5"
#define UNIO_Z80SIO_TAG			"z15"
#define UNIO_Z80PIO_TAG			"z13"
#define UNIO_CENTRONICS1_TAG	"n3"
#define UNIO_CENTRONICS2_TAG	"n4"

class prof80_state : public driver_device
{
public:
	prof80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG),
		  m_rtc(*this, UPD1990A_TAG),
		  m_fdc(*this, UPD765_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<upd1990a_device> m_rtc;
	required_device<device_t> m_fdc;

	/* memory state */
	UINT8 mmu[16];			/* MMU block register */
	int init;				/* MMU enable */

	/* RTC state */
	int c0;
	int c1;
	int c2;

	/* floppy state */
	int	fdc_index;			/* floppy index hole sensor */
	int motor;				/* floppy motor */

	/* GRIP state */
	UINT8 gripd;			/* GRIP data */
	UINT8 gripc;			/* GRIP status */

	/* timers */
	emu_timer	*floppy_motor_off_timer;

/* ------------------------------------------------------------------------ */

	/* sound state */
	int vol0;
	int vol1;

	/* keyboard state */
	UINT8 keydata;			/* keyboard data */
	int kbf;				/* keyboard buffer full */

	/* video state */
	UINT8 *video_ram;		/* video RAM */
	int lps;				/* light pen sense */
	int page;				/* video page */
	int flash;				/* flash */

	/* devices */
	device_t *mc6845;
	device_t *ppi8255;
	device_t *z80sti;
	device_t *centronics;
};

#endif
