/*********************************************************************

    uimess.c

    MESS supplement to ui.c.

*********************************************************************/

#include "emu.h"
#include "uimenu.h"
#include "uimess.h"
#include "uiinput.h"
#include "input.h"
#include "devices/cassette.h"
#include "devices/bitbngr.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
#define TAPECMD_NULL			((void *) 0x0000)
#define TAPECMD_STOP			((void *) 0x0001)
#define TAPECMD_PLAY			((void *) 0x0002)
#define TAPECMD_RECORD			((void *) 0x0003)
#define TAPECMD_REWIND			((void *) 0x0004)
#define TAPECMD_FAST_FORWARD		((void *) 0x0005)
#define TAPECMD_SLIDER			((void *) 0x0006)
#define TAPECMD_SELECT			((void *) 0x0007)

#define BITBANGERCMD_SELECT			((void *) 0x0000)
#define BITBANGERCMD_MODE			((void *) 0x0001)
#define BITBANGERCMD_BAUD			((void *) 0x0002)
#define BITBANGERCMD_TUNE			((void *) 0x0003)


typedef struct _tape_control_menu_state tape_control_menu_state;
struct _tape_control_menu_state
{
	int index;
	device_image_interface *device;
};



typedef struct _bitbanger_control_menu_state bitbanger_control_menu_state;
struct _bitbanger_control_menu_state
{
	int index;
	device_image_interface *device;
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ui_mess_main_menu_populate - populate MESS-specific menus
-------------------------------------------------*/

void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu)
{
	/* add tape control menu */
	if (machine->m_devicelist.first(CASSETTE))
		ui_menu_item_append(menu, "Tape Control", NULL, 0, (void*)ui_mess_menu_tape_control);

	/* add bitbanger control menu */
	if (machine->m_devicelist.first(BITBANGER))
		ui_menu_item_append(menu, "Bitbanger Control", NULL, 0, (void*)ui_mess_menu_bitbanger_control);
}


/*-------------------------------------------------
    cassette_count - returns the number of cassette
    devices in the machine
-------------------------------------------------*/

INLINE int cassette_count( running_machine *machine )
{
	int count = 0;
	running_device *device = machine->m_devicelist.first(CASSETTE);

	while ( device )
	{
		count++;
		device = device->typenext();
	}
	return count;
}

/*-------------------------------------------------
    bitbanger_count - returns the number of bitbanger
    devices in the machine
-------------------------------------------------*/

INLINE int bitbanger_count( running_machine *machine )
{
	int count = 0;
	running_device *device = machine->m_devicelist.first(BITBANGER);

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

	if (menustate->device->exists())
	{
		double t0, t1;
		UINT32 tapeflags = 0;

		t0 = cassette_get_position(&menustate->device->device());
		t1 = cassette_get_length(&menustate->device->device());

		if (t1 > 0)
		{
			if (t0 > 0)
				tapeflags |= MENU_FLAG_LEFT_ARROW;
			if (t0 < t1)
				tapeflags |= MENU_FLAG_RIGHT_ARROW;
		}

		/* name of tape */
		ui_menu_item_append(menu, menustate->device->image_config().devconfig().name(), menustate->device->filename(), flags, TAPECMD_SELECT);

		/* state */
		tapecontrol_gettime(&timepos, &menustate->device->device(), NULL, NULL);
		state = cassette_get_state(&menustate->device->device());
		ui_menu_item_append(
			menu,
			(state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	"stopped"
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "playing" : "(playing)")
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? "recording" : "(recording)")
					),
			astring_c(&timepos),
			tapeflags,
			TAPECMD_SLIDER);

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
    menu_bitbanger_control_populate - populates the
    main bitbanger control menu
-------------------------------------------------*/

static void menu_bitbanger_control_populate(running_machine *machine, ui_menu *menu, bitbanger_control_menu_state *menustate)
{
	int count = bitbanger_count(machine);
	UINT32 flags = 0, mode_flags = 0, baud_flags = 0, tune_flags = 0;

	if( count > 0 )
	{
		if( menustate->index == (count-1) )
			flags |= MENU_FLAG_LEFT_ARROW;
		else
			flags |= MENU_FLAG_RIGHT_ARROW;
	}

   if (bitbanger_inc_mode(&menustate->device->device(), TRUE))
      mode_flags |= MENU_FLAG_RIGHT_ARROW;

   if (bitbanger_dec_mode(&menustate->device->device(), TRUE))
      mode_flags |= MENU_FLAG_LEFT_ARROW;

   if (bitbanger_inc_baud(&menustate->device->device(), TRUE))
      baud_flags |= MENU_FLAG_RIGHT_ARROW;

   if (bitbanger_dec_baud(&menustate->device->device(), TRUE))
      baud_flags |= MENU_FLAG_LEFT_ARROW;

   if (bitbanger_inc_tune(&menustate->device->device(), TRUE))
      tune_flags |= MENU_FLAG_RIGHT_ARROW;

   if (bitbanger_dec_tune(&menustate->device->device(), TRUE))
      tune_flags |= MENU_FLAG_LEFT_ARROW;


	if (menustate->device->exists())
	{
		/* name of bitbanger file */
		ui_menu_item_append(menu, menustate->device->image_config().devconfig().name(), menustate->device->filename(), flags, BITBANGERCMD_SELECT);
		ui_menu_item_append(menu, "Device Mode:", bitbanger_mode_string(&menustate->device->device()), mode_flags, BITBANGERCMD_MODE);
		ui_menu_item_append(menu, "Baud:", bitbanger_baud_string(&menustate->device->device()), baud_flags, BITBANGERCMD_BAUD);
		ui_menu_item_append(menu, "Baud Tune:", bitbanger_tune_string(&menustate->device->device()), tune_flags, BITBANGERCMD_TUNE);
		ui_menu_item_append(menu, "Protocol:", "8-1-N", 0, NULL);
	}
	else
	{
		/* no tape loaded */
		ui_menu_item_append(menu, "No Bitbanger Image loaded", NULL, flags, NULL);
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
		device_image_interface *device = NULL;
		for (bool gotone = machine->m_devicelist.first(device); gotone; gotone = device->next(device))
		{
			if(device->device().type() == CASSETTE) {
				if (index==0) break;
				index--;
			}
		}
		menustate->device = device;
		ui_menu_reset(menu, (ui_menu_reset_options)0);
	}

	/* rebuild the menu - we have to do this so that the counter updates */
	ui_menu_reset(menu, UI_MENU_RESET_REMEMBER_POSITION);
	menu_tape_control_populate(machine, menu, (tape_control_menu_state*)state);

	/* process the menu */
	event = ui_menu_process(machine, menu, UI_MENU_PROCESS_LR_REPEAT);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				if (event->itemref==TAPECMD_SLIDER)
					cassette_seek(&menustate->device->device(), -1, SEEK_CUR);
				else
				if (event->itemref==TAPECMD_SELECT)
				{
					/* left arrow - rotate left through cassette devices */
					if (menustate->index > 0)
						menustate->index--;
					else
						menustate->index = cassette_count(machine) - 1;
					menustate->device = NULL;
				}
				break;

			case IPT_UI_RIGHT:
				if (event->itemref==TAPECMD_SLIDER)
					cassette_seek(&menustate->device->device(), +1, SEEK_CUR);
				else
				if (event->itemref==TAPECMD_SELECT)
				{
					/* right arrow - rotate right through cassette devices */
					if (menustate->index < cassette_count(machine) - 1)
						menustate->index++;
					else
						menustate->index = 0;
					menustate->device = NULL;
				}
				break;

			case IPT_UI_SELECT:
				{
					if (event->itemref==TAPECMD_STOP)
						cassette_change_state(&menustate->device->device(), CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_PLAY)
						cassette_change_state(&menustate->device->device(), CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_RECORD)
						cassette_change_state(&menustate->device->device(), CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
					else
					if (event->itemref==TAPECMD_REWIND)
						cassette_seek(&menustate->device->device(), -30, SEEK_CUR);
					else
					if (event->itemref==TAPECMD_FAST_FORWARD)
						cassette_seek(&menustate->device->device(), 30, SEEK_CUR);
					else
					if (event->itemref==TAPECMD_SLIDER)
						cassette_seek(&menustate->device->device(), 0, SEEK_SET);
				}
				break;
		}
	}
}


/*-------------------------------------------------
    menu_bitbanger_control - main bitbanger
    control menu
-------------------------------------------------*/

void ui_mess_menu_bitbanger_control(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	bitbanger_control_menu_state *menustate;
	const ui_menu_event *event;

	/* if no state, allocate some */
	if (state == NULL)
		state = ui_menu_alloc_state(menu, sizeof(*menustate), NULL);
	menustate = (bitbanger_control_menu_state *) state;

	/* do we have to load the device? */
	if (menustate->device == NULL)
	{
		int index = menustate->index;
		device_image_interface *device = NULL;
		for (bool gotone = machine->m_devicelist.first(device); gotone; gotone = device->next(device))
		{
			if(device->device().type() == BITBANGER) {
				if (index==0) break;
				index--;
			}
		}
		menustate->device = device;
		ui_menu_reset(menu, (ui_menu_reset_options)0);
	}

	/* rebuild the menu */
	ui_menu_reset(menu, UI_MENU_RESET_REMEMBER_POSITION);
	menu_bitbanger_control_populate(machine, menu, (bitbanger_control_menu_state*)state);

	/* process the menu */
	event = ui_menu_process(machine, menu, UI_MENU_PROCESS_LR_REPEAT);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				if (event->itemref==BITBANGERCMD_SELECT)
				{
					/* left arrow - rotate left through cassette devices */
					if (menustate->index > 0)
						menustate->index--;
					else
						menustate->index = bitbanger_count(machine) - 1;
					menustate->device = NULL;
				}
				else if (event->itemref==BITBANGERCMD_MODE)
				{
				   bitbanger_dec_mode(&menustate->device->device(), FALSE);
				}
				else if (event->itemref==BITBANGERCMD_BAUD)
				{
				   bitbanger_dec_baud(&menustate->device->device(), FALSE);
				}
				else if (event->itemref==BITBANGERCMD_TUNE)
				{
				   bitbanger_dec_tune(&menustate->device->device(), FALSE);
				}
				break;

			case IPT_UI_RIGHT:
				if (event->itemref==BITBANGERCMD_SELECT)
				{
					/* right arrow - rotate right through cassette devices */
					if (menustate->index < bitbanger_count(machine) - 1)
						menustate->index++;
					else
						menustate->index = 0;
					menustate->device = NULL;
				}
				else if (event->itemref==BITBANGERCMD_MODE)
				{
				   bitbanger_inc_mode(&menustate->device->device(), FALSE);
				}
				else if (event->itemref==BITBANGERCMD_BAUD)
				{
				   bitbanger_inc_baud(&menustate->device->device(), FALSE);
				}
				else if (event->itemref==BITBANGERCMD_TUNE)
				{
				   bitbanger_inc_tune(&menustate->device->device(), FALSE);
				}
				break;
		}
	}
}

