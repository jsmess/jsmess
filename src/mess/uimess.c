/*********************************************************************

	uimess.c

	MESS supplement to ui.c.

*********************************************************************/

#include "mame.h"
#include "mslegacy.h"
#include "uimenu.h"
#include "uimess.h"
#include "uiinput.h"
#include "input.h"


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static int ui_active = 0;



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

int mess_ui_active(void)
{
	return ui_active;
}

void mess_ui_update(running_machine *machine)
{
	static int ui_toggle_key = 0;

	int toggled = 0;
	const device_config *dev;

	/* traditional MESS interface */
	if (machine->gamedrv->flags & GAME_COMPUTER)
	{
		if (ui_input_pressed(machine, IPT_UI_TOGGLE_UI) )
		{
			if( !ui_toggle_key )
			{
				toggled = 1;
				ui_toggle_key = 1;
				ui_active = !ui_active;
			}
		}
		else
		{
			if (ui_toggle_key)
			{
				toggled = 1;
			}
			ui_toggle_key = 0;
		}

		if (ui_active)
		{
			if (toggled)
			{
				popmessage("%s\n%s\n%s\n%s\n%s\n%s\n",
					ui_getstring(UI_keyb1),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb3),
					ui_getstring(UI_keyb5),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb7));
			}
		}
		else
		{
			if (toggled)
			{
				popmessage("%s\n%s\n%s\n%s\n%s\n%s\n",
					ui_getstring(UI_keyb1),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb4),
					ui_getstring(UI_keyb6),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb7));
			}
		}
	}

	/* run display routine for device */
	if (mame_get_phase(machine) == MAME_PHASE_RUNNING)
	{
		for (dev = image_device_first(machine->config); dev != NULL; dev = image_device_next(dev))
		{
			device_display_func display = (device_display_func) device_get_info_fct(dev, DEVINFO_FCT_DISPLAY);
			if (display != NULL)
			{
				display(dev);
			}
		}
	}
}



/*-------------------------------------------------
    image_info_astring - populate an allocated
    string with the image info text
-------------------------------------------------*/

static astring *image_info_astring(running_machine *machine, astring *string)
{
	const device_config *img;

	astring_printf(string, "%s\n\n", machine->gamedrv->description);

	if (mess_ram_size > 0)
	{
		char buf2[RAM_STRING_BUFLEN];
		astring_catprintf(string, "RAM: %s\n\n", ram_string(buf2, mess_ram_size));
	}

	for (img = image_device_first(machine->config); img != NULL; img = image_device_next(img))
	{
		const char *name = image_filename(img);
		if (name != NULL)
		{
			const char *base_filename;
			const char *info;
			char *base_filename_noextension;

			base_filename = image_basename(img);
			base_filename_noextension = strip_extension(base_filename);

			/* display device type and filename */
			astring_catprintf(string, "%s: %s\n", image_typename_id(img), base_filename);

			/* display long filename, if present and doesn't correspond to name */
			info = image_longname(img);
			if (info && (!base_filename_noextension || mame_stricmp(info, base_filename_noextension)))
				astring_catprintf(string, "%s\n", info);

			/* display manufacturer, if available */
			info = image_manufacturer(img);
			if (info != NULL)
			{
				astring_catprintf(string, "%s", info);
				info = stripspace(image_year(img));
				if (info && *info)
					astring_catprintf(string, ", %s", info);
				astring_catprintf(string,"\n");
			}

			/* display playable information, if available */
			info = image_playable(img);
			if (info != NULL)
				astring_catprintf(string, "%s\n", info);

			if (base_filename_noextension != NULL)
				free(base_filename_noextension);
		}
		else
		{
			astring_catprintf(string, "%s: ---\n", image_typename_id(img));
		}
	}
	return string;
}



/*-------------------------------------------------
    ui_menu_image_info - menu that shows info on
	all loaded images
-------------------------------------------------*/

void ui_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
	{
		astring *tempstring = image_info_astring(machine, astring_alloc());
		ui_menu_item_append(menu, astring_c(tempstring), NULL, MENU_FLAG_MULTILINE, NULL);
		astring_free(tempstring);
	}

	/* process the menu */
	ui_menu_process(menu, 0);
}



/*-------------------------------------------------
    mess_use_new_ui - determines if the "new ui"
	is in use
-------------------------------------------------*/

/*-------------------------------------------------
    mess_use_new_ui - determines if the "new ui"
	is in use
-------------------------------------------------*/

int mess_use_new_ui(void)
{
#if (defined(WIN32) || defined(_MSC_VER)) && !defined(SDLMAME_WIN32)
	if (options_get_bool(mame_options(), "newui"))
		return TRUE;
#endif
	return FALSE;
}



/*-------------------------------------------------
    mess_disable_builtin_ui - function to determine
	if the builtin UI should be disabled
-------------------------------------------------*/

int mess_disable_builtin_ui(running_machine *machine)
{
	return mess_use_new_ui() || ((machine->gamedrv->flags & GAME_COMPUTER) && !mess_ui_active());
}



/*-------------------------------------------------
    ui_paste - does a paste from the keyboard
-------------------------------------------------*/

void ui_paste(running_machine *machine)
{
	/* retrieve the clipboard text */
	char *text = osd_get_clipboard_text();

	/* was a result returned? */
	if (text != NULL)
	{
		/* post the text */
		inputx_post_utf8(machine, text);

		/* free the string */
		free(text);
	}
}
