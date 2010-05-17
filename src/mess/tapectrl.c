/*********************************************************************

  tapectrl.c

  MESS's clunky built-in tape control

*********************************************************************/

#include "emu.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "devices/cassette.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
#define TAPECMD_NULL			((void *) 0x0000)
#define TAPECMD_STOP			((void *) 0x0001)
#define TAPECMD_PLAY			((void *) 0x0002)
#define TAPECMD_RECORD			((void *) 0x0003)
#define TAPECMD_REWIND			((void *) 0x0004)
#define TAPECMD_FAST_FORWARD	((void *) 0x0005)



typedef struct _tape_control_menu_state tape_control_menu_state;
struct _tape_control_menu_state
{
	int index;
	running_device *device;
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    cassette_count - returns the number of cassette
    devices in the machine
-------------------------------------------------*/

INLINE int cassette_count( running_machine *machine )
{
	int count = 0;
	running_device *device = machine->devicelist.first(CASSETTE );

	while ( device )
	{
		count++;
		device = device->typenext();
	}
	return count;
}

/*-------------------------------------------------
    tapecontrol_gettime - returns a textual
    representation of the time
-------------------------------------------------*/

astring *tapecontrol_gettime(astring *dest, running_device *device, int *curpos, int *endpos)
{
	double t0, t1;

	t0 = cassette_get_position(device);
	t1 = cassette_get_length(device);

	if (t1)
		astring_printf(dest, "%04d/%04d", (int) t0, (int) t1);
	else
		astring_printf(dest, "%04d/%04d", 0, (int) t1);

	if (curpos != NULL)
		*curpos = t0;
	if (endpos != NULL)
		*endpos = t1;

	return dest;
}



/*-------------------------------------------------
    menu_tape_control_populate - populates the
    main tape control menu
-------------------------------------------------*/

static void menu_tape_control_populate(running_machine *machine, ui_menu *menu, tape_control_menu_state *menustate)
{
	astring timepos;
	cassette_state state;
	int count = cassette_count(machine);
	UINT32 flags = 0;

	if( count > 0 )
	{
		if( menustate->index == (count-1) )
			flags |= MENU_FLAG_LEFT_ARROW;
		else
			flags |= MENU_FLAG_RIGHT_ARROW;
	}

	if (image_exists(menustate->device))
	{
		/* name of tape */
		ui_menu_item_append(menu, image_typename_id(menustate->device), image_filename(menustate->device), flags, NULL);

		/* state */
		tapecontrol_gettime(&timepos, menustate->device, NULL, NULL);
		state = cassette_get_state(menustate->device);
		ui_menu_item_append(
			menu,
			(state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	"stopped"
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "playing" : "(playing)")
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "recording" : "(recording)")
					),
			astring_c(&timepos),
			0,
			NULL);

		/* pause or stop */
		ui_menu_item_append(menu, "Pause/Stop", NULL, 0, TAPECMD_STOP);

		/* play */
		ui_menu_item_append(menu, "Play", NULL, 0, TAPECMD_PLAY);

		/* record */
		ui_menu_item_append(menu, "Record", NULL, 0, TAPECMD_RECORD);

		/* rewind */
		ui_menu_item_append(menu, "Rewind", NULL, 0, TAPECMD_REWIND);

		/* fast forward */
		ui_menu_item_append(menu, "Fast Forward", NULL, 0, TAPECMD_FAST_FORWARD);
	}
	else
	{
		/* no tape loaded */
		ui_menu_item_append(menu, "No Tape Image loaded", NULL, flags, NULL);
	}
}


/*-------------------------------------------------
    menu_tape_control - main tape control menu
-------------------------------------------------*/

void ui_mess_menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	tape_control_menu_state *menustate;
	const ui_menu_event *event;

	/* if no state, allocate some */
	if (state == NULL)
		state = ui_menu_alloc_state(menu, sizeof(*menustate), NULL);
	menustate = (tape_control_menu_state *) state;

	/* do we have to load the device? */
	if (menustate->device == NULL)
	{
		int index = menustate->index;
		running_device *device = machine->devicelist.first( CASSETTE );

		while ( index > 0 && device )
		{
			device = device->typenext();
			index--;
		}
		menustate->device = device;
		ui_menu_reset(menu, (ui_menu_reset_options)0);
	}

	/* rebuild the menu - we have to do this so that the counter updates */
	ui_menu_reset(menu, UI_MENU_RESET_REMEMBER_POSITION);
	menu_tape_control_populate(machine, menu, (tape_control_menu_state*)state);

	/* process the menu */
	event = ui_menu_process(machine, menu, 0);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				/* left arrow - rotate left through cassette devices */
				if (menustate->index > 0)
					menustate->index--;
				else
					menustate->index = cassette_count(machine) - 1;
				menustate->device = NULL;
				break;

			case IPT_UI_RIGHT:
				/* right arrow - rotate right through cassette devices */
				if (menustate->index < cassette_count(machine) - 1)
					menustate->index++;
				else
					menustate->index = 0;
				menustate->device = NULL;
				break;

			case IPT_UI_SELECT:
				{
					if (event->itemref==TAPECMD_NULL) {
					}
					else if (event->itemref==TAPECMD_STOP) {
						cassette_change_state(menustate->device, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
					}
					else if (event->itemref==TAPECMD_PLAY) {
						cassette_change_state(menustate->device, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
					}
					else if (event->itemref==TAPECMD_RECORD) {
						cassette_change_state(menustate->device, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
					}
					else if (event->itemref==TAPECMD_REWIND) {
						cassette_seek(menustate->device, -60, SEEK_CUR);
					}
					else if (event->itemref==TAPECMD_FAST_FORWARD) {
						cassette_seek(menustate->device, +60, SEEK_CUR);
					}
				}
				break;
		}
	}
}
