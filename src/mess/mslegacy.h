/*********************************************************************

    mslegacy.h

    Defines since removed from MAME, but kept in MESS for legacy
    reasons

*********************************************************************/

#ifndef MSLEGACY_H
#define MSLEGACY_H

#include "driver.h"

enum
{
	UI_OK,
	UI_off,
	UI_on,
	UI_dipswitches,
	UI_configuration,
	UI_analogcontrols,
	UI_keyjoyspeed,
	UI_centerspeed,
	UI_reverse,
	UI_sensitivity,

	UI_last_mame_entry,
	UI_keyb1 = UI_last_mame_entry,
	UI_keyb2,
	UI_keyb3,
	UI_keyb4,
	UI_keyb5,
	UI_keyb6,
	UI_keyb7,

	UI_notapeimageloaded,
	UI_recording,
	UI_playing,
	UI_recording_inhibited,
	UI_playing_inhibited,
	UI_stopped,
	UI_pauseorstop,
	UI_record,
	UI_play,
	UI_rewind,
	UI_fastforward,
	UI_mount,
	UI_create,
	UI_unmount,
	UI_emptyslot,
	UI_categories,
	UI_quitfileselector,
	UI_filespecification,	/* IMPORTANT: be careful to ensure that the following */
	UI_cartridge,			/* device list matches the order found in device.h    */
	UI_floppydisk,			/* and is ALWAYS placed after UI_filespecification    */
	UI_harddisk,
	UI_cylinder,
	UI_cassette,
	UI_punchcard,
	UI_punchtape,
	UI_printer,
	UI_serial,
	UI_parallel,
	UI_snapshot,
	UI_quickload,
	UI_memcard,
	UI_cdrom
};



/* uitext */
const char * ui_getstring (int string_num);

#endif /* MSLEGACY_H */
