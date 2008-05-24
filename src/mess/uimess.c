/*********************************************************************

	uimess.c

	MESS supplement to ui.c.

*********************************************************************/

#include "mame.h"
#include "mslegacy.h"
#include "uimenu.h"
#include "uimess.h"
#include "input.h"

//int mess_pause_for_ui = 0;
static int ui_active = 0;

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
		if( input_ui_pressed(machine, IPT_UI_TOGGLE_UI) )
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



/*************************************
 *
 *  Image info
 *
 *************************************/

int ui_sprintf_image_info(running_machine *machine, char *buf)
{
	char *dst = buf;
	const device_config *img;

	dst += sprintf(dst, "%s\n\n", machine->gamedrv->description);

	if (mess_ram_size > 0)
	{
		char buf2[RAM_STRING_BUFLEN];
		dst += sprintf(dst, "RAM: %s\n\n", ram_string(buf2, mess_ram_size));
	}

	for (img = image_device_first(machine->config); img != NULL; img = image_device_next(img))
	{
		const char *name = image_filename(img);
		if( name )
		{
			const char *base_filename;
			const char *info;
			char *base_filename_noextension;

			base_filename = image_basename(img);
			base_filename_noextension = strip_extension(base_filename);

			/* display device type and filename */
			dst += sprintf(dst,"%s: %s\n", image_typename_id(img), base_filename);

			/* display long filename, if present and doesn't correspond to name */
			info = image_longname(img);
			if (info && (!base_filename_noextension || mame_stricmp(info, base_filename_noextension)))
				dst += sprintf(dst,"%s\n", info);

			/* display manufacturer, if available */
			info = image_manufacturer(img);
			if (info)
			{
				dst += sprintf(dst,"%s", info);
				info = stripspace(image_year(img));
				if (info && *info)
					dst += sprintf(dst,", %s", info);
				dst += sprintf(dst,"\n");
			}

			/* display playable information, if available */
			info = image_playable(img);
			if (info)
				dst += sprintf(dst,"%s\n", info);

			if (base_filename_noextension)
				free(base_filename_noextension);
		}
		else
		{
			dst += sprintf(dst,"%s: ---\n", image_typename_id(img));
		}
	}
	return dst - buf;
}



UINT32 ui_menu_image_info(running_machine *machine, UINT32 state)
{
	char buf[2048];
	char *bufptr = buf;
	UINT32 selected = 0;

	/* add the game info */
	bufptr += ui_sprintf_image_info(machine, bufptr);

	/* make it look like a menu */
	bufptr += sprintf(bufptr, "\n\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_returntomain), ui_getstring(UI_righthilight));

	/* draw the text */
	ui_draw_message_window(buf);

	/* handle the keys */
	ui_menu_generic_keys(machine, &selected, 1, 0);
	return selected;
}



int mess_use_new_ui(void)
{
#if (defined(WIN32) || defined(_MSC_VER)) && !defined(SDLMAME_WIN32)
	if (options_get_bool(mame_options(), "newui"))
		return TRUE;
#endif
	return FALSE;
}



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
