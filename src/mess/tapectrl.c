/*********************************************************************

  tapectrl.c

  MESS's clunky built-in tape control

*********************************************************************/

#include "driver.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "uitext.h"
#include "devices/cassette.h"

void tapecontrol_gettime(char *timepos, size_t timepos_size, mess_image *img, int *curpos, int *endpos)
{
	double t0, t1;

	t0 = cassette_get_position(img);
	t1 = cassette_get_length(img);

	if (t1)
		snprintf(timepos, timepos_size, "%04d/%04d", (int) t0, (int) t1);
	else
		snprintf(timepos, timepos_size, "%04d/%04d", 0, (int) t1);

	if (curpos)
		*curpos = t0;
	if (endpos)
		*endpos = t1;
}

int tapecontrol(int selected)
{
	static int id = 0;
	char timepos[32];
	ui_menu_item menu_item[40];
	char name[64];
	mess_image *img;

	int sel;
	int total;
	int arrowize;
	cassette_state state;

	total = 0;
	sel = selected - 1;

	img = image_from_devtype_and_index(IO_CASSETTE, id);
	if ( !image_filename(img) )
	{
		sprintf(name, "\t%s\n\n\t", ui_getstring(UI_notapeimageloaded) );
		strcat(name, ui_getstring(UI_lefthilight));
		strcat(name, " ");
		strcat(name, ui_getstring(UI_returntomain));
		strcat(name, " ");
		strcat(name, ui_getstring(UI_righthilight));
		ui_draw_message_window(name);
		
		if (input_ui_pressed(IPT_UI_SELECT) || input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;
		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;

		return sel + 1;
	}

	strcpy( name, image_typename_id(img) );
	menu_item[total].text = name;
	menu_item[total].subtext = image_filename(img);
	menu_item[total].flags = 0;
	total++;

	tapecontrol_gettime(timepos, sizeof(timepos) / sizeof(timepos[0]), img, NULL, NULL);

	state = cassette_get_state(img);
	menu_item[total].text =
		ui_getstring(
			(state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	UI_stopped
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_playing : UI_playing_inhibited)
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_recording : UI_recording_inhibited)
					));

	menu_item[total].subtext = timepos;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_pauseorstop);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_play);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_record);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_rewind);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_fastforward);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	menu_item[total].text = ui_getstring(UI_returntomain);
	menu_item[total].subtext = 0;
	menu_item[total].flags = 0;
	total++;

	arrowize = 0;
	if (sel < total - 1)
		arrowize = 2;

	if (sel > 255)  /* are we waiting for a new key? */
	{
		/* display the menu */
		ui_menu_draw(menu_item, total, sel & 0xff, NULL);
		return sel + 1;
	}

	ui_menu_draw(menu_item, total, sel, NULL);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
	{
		if (sel < total - 1) sel++;
		else sel = 0;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		if (sel > 0) sel--;
		else sel = total - 1;
	}


	if (input_ui_pressed(IPT_UI_LEFT))
	{
		switch (sel)
		{
		case 0:
			id--;
			if (id < 0) id = device_count(IO_CASSETTE)-1;
			break;
		}
	}

	if (input_ui_pressed(IPT_UI_RIGHT))
	{
		switch (sel)
		{
		case 0:
			id++;
			if (id > device_count(IO_CASSETTE)-1) id = 0;
			break;
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1)
			sel = -1;
		else
		{
			img = image_from_devtype_and_index(IO_CASSETTE, id);
			switch (sel)
			{
			case 0:
				id = (id + 1) % device_count(IO_CASSETTE);
				break;
			case 2:
				/* Pause/stop */
				cassette_change_state(img, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
				break;
			case 3:
				/* Play */
				cassette_change_state(img, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
				break;
			case 4:
				/* Record */
				cassette_change_state(img, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
				break;
			case 5:
				/* Rewind */
				cassette_seek(img, -1, SEEK_CUR);
				break;
			case 6:
				/* Fast forward */
				cassette_seek(img, +1, SEEK_CUR);
				break;
			}
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	return sel + 1;
}
