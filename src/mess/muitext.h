#ifndef MUI_TEXT_H
#define MUI_TEXT_H

enum
{
	UI_comp1 = UI_last_mame_entry,
	UI_comp2,
	UI_keyb1,
	UI_keyb2,
	UI_keyb3,
	UI_keyb4,
	UI_keyb5,
	UI_keyb6,
	UI_keyb7,

	UI_imageinfo,
	UI_filemanager,
	UI_tapecontrol,

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

#endif /* MUI_TEXT_H */
