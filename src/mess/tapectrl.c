/*********************************************************************

  tapectrl.c

  MESS's clunky built-in tape control

*********************************************************************/

#include "driver.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "mslegacy.h"
#include "devices/cassette.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum _tape_control_command
{
	TAPECMD_NULL = 0,
	TAPECMD_STOP,
	TAPECMD_PLAY,
	TAPECMD_RECORD,
	TAPECMD_REWIND,
	TAPECMD_FAST_FORWARD
};
typedef enum _tape_control_command tape_control_command;



typedef struct _tape_control_menu_state tape_control_menu_state;
struct _tape_control_menu_state
{
	int index;
	const device_config *device;
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    tapecontrol_gettime - returns a textual
	representation of the time
-------------------------------------------------*/

astring *tapecontrol_gettime(astring *dest, const device_config *device, int *curpos, int *endpos)
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
	astring *timepos = astring_alloc();
	cassette_state state;

	if (image_exists(menustate->device))
	{
		/* name of tape */
		ui_menu_item_append(menu, image_typename_id(menustate->device), image_filename(menustate->device), 0, NULL);

		/* state */
		tapecontrol_gettime(timepos, menustate->device, NULL, NULL);
		state = cassette_get_state(menustate->device);
		ui_menu_item_append(
			menu,
			ui_getstring((state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	UI_stopped
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_playing : UI_playing_inhibited)
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_recording : UI_recording_inhibited)
					)),
			astring_c(timepos),
			0,
			NULL);

		/* pause or stop */
		ui_menu_item_append(menu, ui_getstring(UI_pauseorstop), NULL, 0, (void *) TAPECMD_STOP);

		/* play */
		ui_menu_item_append(menu, ui_getstring(UI_play), NULL, 0, (void *) TAPECMD_PLAY);

		/* record */
		ui_menu_item_append(menu, ui_getstring(UI_record), NULL, 0, (void *) TAPECMD_RECORD);

		/* rewind */
		ui_menu_item_append(menu, ui_getstring(UI_rewind), NULL, 0, (void *) TAPECMD_REWIND);

		/* fast forward */
		ui_menu_item_append(menu, ui_getstring(UI_fastforward), NULL, 0, (void *) TAPECMD_FAST_FORWARD);
	}
	else
	{
		/* no tape loaded */
		ui_menu_item_append(menu, ui_getstring(UI_notapeimageloaded), NULL, 0, NULL);
	}

	if (timepos != NULL)
		astring_free(timepos);
}


/*-------------------------------------------------
    menu_tape_control - main tape control menu
-------------------------------------------------*/

void menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state)
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
		menustate->device = image_from_devtype_and_index(IO_CASSETTE, menustate->index);
		ui_menu_reset(menu, 0);
	}

	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
		menu_tape_control_populate(machine, menu, state);

	/* process the menu */
	event = ui_menu_process(menu, 0);
	if (event != NULL)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				/* left arrow - rotate left through cassette devices */
				if (menustate->index > 0)
					menustate->index--;
				else
					menustate->index = device_count(machine, IO_CASSETTE) - 1;
				menustate->device = NULL;
				break;

			case IPT_UI_RIGHT:
				/* right arrow - rotate right through cassette devices */
				if (menustate->index < device_count(machine, IO_CASSETTE) - 1)
					menustate->index++;
				else
					menustate->index = 0;
				menustate->device = NULL;
				break;

			case IPT_UI_SELECT:
				switch((tape_control_command) event->itemref)
				{
					case TAPECMD_NULL:
						break;

					case TAPECMD_STOP:
						cassette_change_state(menustate->device, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
						break;

					case TAPECMD_PLAY:
						cassette_change_state(menustate->device, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
						break;

					case TAPECMD_RECORD:
						cassette_change_state(menustate->device, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
						break;

					case TAPECMD_REWIND:
						cassette_seek(menustate->device, -1, SEEK_CUR);
						break;

					case TAPECMD_FAST_FORWARD:
						cassette_seek(menustate->device, +1, SEEK_CUR);
						break;
				}
				break;
		}
	}
}
