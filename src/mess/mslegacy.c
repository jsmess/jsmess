/*********************************************************************

	mslegacy.c

	Defines since removed from MAME, but kept in MESS for legacy
	reasons

*********************************************************************/

#include "driver.h"
#include "mslegacy.h"


static const char *const mess_default_text[] =
{
	"OK",
	"Off",
	"On",
	"Dip Switches",
	"Driver Configuration",
	"Analog Controls",
	"Digital Speed",
	"Autocenter Speed",
	"Reverse",
	"Sensitivity",
	"\xc2\xab",
	"\xc2\xbb",
	"Return to Main Menu",

	"Keyboard Emulation Status",
	"-------------------------",
	"Mode: PARTIAL Emulation",
	"Mode: FULL Emulation",
	"UI:   Enabled",
	"UI:   Disabled",
	"**Use ScrLock to toggle**",

	"Image Information",
	"File Manager",
	"Tape Control",

	"No Tape Image loaded",
	"recording",
	"playing",
	"(recording)",
	"(playing)",
	"stopped",
	"Pause/Stop",
	"Record",
	"Play",
	"Rewind",
	"Fast Forward",
	"Mount...",
	"Create...",
	"Unmount",
	"[empty slot]",
	"Input Devices",
	"Quit Fileselector",
	"File Specification",	/* IMPORTANT: be careful to ensure that the following */
	"Cartridge",		/* device list matches the order found in device.h    */
	"Floppy Disk",		/* and is ALWAYS placed after "File Specification"    */
	"Hard Disk",
	"Cylinder",
	"Cassette",
	"Punched Card",
	"Punched Tape",
	"Printer",
	"Serial Port",
	"Parallel Port",
	"Snapshot",
	"Quickload",
	"Memory Card",
	"CD-ROM"
};



/***************************************************************************
    PALETTE
***************************************************************************/

void palette_set_colors_rgb(running_machine *machine, pen_t color_base, const UINT8 *colors, int color_count)
{
	while (color_count--)
	{
		UINT8 r = *colors++;
		UINT8 g = *colors++;
		UINT8 b = *colors++;
		palette_set_color_rgb(machine, color_base++, r, g, b);
	}
}



/***************************************************************************
    UI TEXT
***************************************************************************/

const char * ui_getstring (int string_num)
{
	return mess_default_text[string_num];
}
