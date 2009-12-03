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
#include "devices/cassette.h"


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

/* list of natural keyboard keys that are not associated with UI_EVENT_CHARs */
static const input_item_id non_char_keys[] =
{
	ITEM_ID_ESC,
	ITEM_ID_F1,
	ITEM_ID_F2,
	ITEM_ID_F3,
	ITEM_ID_F4,
	ITEM_ID_F5,
	ITEM_ID_F6,
	ITEM_ID_F7,
	ITEM_ID_F8,
	ITEM_ID_F9,
	ITEM_ID_F10,
	ITEM_ID_F11,
	ITEM_ID_F12,
	ITEM_ID_NUMLOCK,
	ITEM_ID_0_PAD,
	ITEM_ID_1_PAD,
	ITEM_ID_2_PAD,
	ITEM_ID_3_PAD,
	ITEM_ID_4_PAD,
	ITEM_ID_5_PAD,
	ITEM_ID_6_PAD,
	ITEM_ID_7_PAD,
	ITEM_ID_8_PAD,
	ITEM_ID_9_PAD,
	ITEM_ID_DEL_PAD,
	ITEM_ID_PLUS_PAD,
	ITEM_ID_MINUS_PAD,
	ITEM_ID_INSERT,
	ITEM_ID_DEL,
	ITEM_ID_HOME,
	ITEM_ID_END,
	ITEM_ID_PGUP,
	ITEM_ID_PGDN,
	ITEM_ID_UP,
	ITEM_ID_DOWN,
	ITEM_ID_LEFT,
	ITEM_ID_RIGHT,
	ITEM_ID_PAUSE,
	ITEM_ID_CANCEL
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _ui_mess_private
{
	int active;
	int use_natural_keyboard;
	UINT8 non_char_keys_down[(ARRAY_LENGTH(non_char_keys) + 7) / 8];
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ui_mess_init - initialize the MESS-specific
	UI
-------------------------------------------------*/

void ui_mess_init(running_machine *machine)
{
	/* allocate memory for our data structure */
	machine->ui_mess_data = auto_alloc_clear(machine, ui_mess_private);

	/* retrieve options */
	machine->ui_mess_data->use_natural_keyboard = options_get_bool(mame_options(), OPTION_NATURAL_KEYBOARD);
}



/*-------------------------------------------------
    process_natural_keyboard - processes any
	natural keyboard input
-------------------------------------------------*/

static void process_natural_keyboard(running_machine *machine)
{
	ui_event event;
	int i, pressed;
	input_item_id itemid;
	input_code code;
	UINT8 *key_down_ptr;
	UINT8 key_down_mask;

	/* loop while we have interesting events */
	while (ui_input_pop_event(machine, &event))
	{
		/* if this was a UI_EVENT_CHAR event, post it */
		if (event.event_type == UI_EVENT_CHAR)
			inputx_postc(machine, event.ch);
	}

	/* process natural keyboard keys that don't get UI_EVENT_CHARs */
	for (i = 0; i < ARRAY_LENGTH(non_char_keys); i++)
	{
		/* identify this keycode */
		itemid = non_char_keys[i];
		code = input_code_from_input_item_id(machine, itemid);

		/* ...and determine if it is pressed */
		pressed = input_code_pressed(machine, code);

		/* figure out whey we are in the key_down map */
		key_down_ptr = &machine->ui_mess_data->non_char_keys_down[i / 8];
		key_down_mask = 1 << (i % 8);

		if (pressed && !(*key_down_ptr & key_down_mask))
		{
			/* this key is now down */
			*key_down_ptr |= key_down_mask;

			/* post the key */
			inputx_postc(machine, UCHAR_MAMEKEY_BEGIN + code);
		}
		else if (!pressed && (*key_down_ptr & key_down_mask))
		{
			/* this key is now up */
			*key_down_ptr &= ~key_down_mask;
		}
	}
}



/*-------------------------------------------------
    ui_mess_handler_ingame - function to determine
	if the builtin UI should be disabled
-------------------------------------------------*/

int ui_mess_handler_ingame(running_machine *machine)
{
	int ui_disabled;
	const device_config *dev;

	/* run display routine for devices */
	if (mame_get_phase(machine) == MAME_PHASE_RUNNING)
	{
		for (dev = image_device_first(machine->config); dev != NULL; dev = image_device_next(dev))
		{
			device_display_func display = (device_display_func) device_get_info_fct(dev, DEVINFO_FCT_DISPLAY);
			if (display != NULL)
			{
				(*display)(dev);
			}
		}
	}

	/* determine if we should disable the rest of the UI */
	ui_disabled = ui_mess_use_new_ui()
		|| ((machine->gamedrv->flags & GAME_COMPUTER) && !machine->ui_mess_data->active);


	/* is ScrLk UI toggling applicable here? */
	if (!ui_mess_use_new_ui() && (machine->gamedrv->flags & GAME_COMPUTER))
	{
		/* are we toggling the UI with ScrLk? */
		if (ui_input_pressed(machine, IPT_UI_TOGGLE_UI))
		{
			/* toggle the UI */
			machine->ui_mess_data->active = !machine->ui_mess_data->active;

			/* display a popup indicating the new status */
			if (machine->ui_mess_data->active)
			{
				ui_popup_time(2, "%s\n%s\n%s\n%s\n%s\n%s\n",
					ui_getstring(UI_keyb1),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb3),
					ui_getstring(UI_keyb5),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb7));
			}
			else
			{
				ui_popup_time(2, "%s\n%s\n%s\n%s\n%s\n%s\n",
					ui_getstring(UI_keyb1),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb4),
					ui_getstring(UI_keyb6),
					ui_getstring(UI_keyb2),
					ui_getstring(UI_keyb7));
			}
		}
	}

	/* is the natural keyboard enabled? */
	if (ui_mess_get_use_natural_keyboard(machine) && (mame_get_phase(machine) == MAME_PHASE_RUNNING))
		process_natural_keyboard(machine);

	/* MESS-specific UI; provided that the UI is not disabled */
	if (!ui_disabled)
	{
		/* paste command */
		if (ui_input_pressed(machine, IPT_UI_PASTE))
			ui_mess_paste(machine);
	}

	return ui_disabled;
}



/*-------------------------------------------------
    image_info_astring - populate an allocated
    string with the image info text
-------------------------------------------------*/

static astring *image_info_astring(running_machine *machine, astring *string)
{
	const device_config *img;

	astring_printf(string, "%s\n\n", machine->gamedrv->description);

#if 0
	if (mess_ram_size > 0)
	{
		char buf2[RAM_STRING_BUFLEN];
		astring_catprintf(string, "RAM: %s\n\n", ram_string(buf2, mess_ram_size));
	}
#endif

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
    ui_mess_menu_image_info - menu that shows info
	on all loaded images
-------------------------------------------------*/

void ui_mess_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
	{
		astring *tempstring = image_info_astring(machine, astring_alloc());
		ui_menu_item_append(menu, astring_c(tempstring), NULL, MENU_FLAG_MULTILINE, NULL);
		astring_free(tempstring);
	}

	/* process the menu */
	ui_menu_process(machine, menu, 0);
}



/*-------------------------------------------------
    ui_mess_use_new_ui - determines if the "new ui"
	is in use
-------------------------------------------------*/

int ui_mess_use_new_ui(void)
{
#if (defined(WIN32) || defined(_MSC_VER)) && !defined(SDLMAME_WIN32)
	if (options_get_bool(mame_options(), "newui"))
		return TRUE;
#endif
	return FALSE;
}



/*-------------------------------------------------
    ui_mess_paste - does a paste from the keyboard
-------------------------------------------------*/

void ui_mess_paste(running_machine *machine)
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



/*-------------------------------------------------
    ui_mess_keyboard_disabled - returns whether
	IPT_KEYBOARD input should be disabled
-------------------------------------------------*/

int ui_mess_keyboard_disabled(running_machine *machine)
{
	return ui_mess_get_use_natural_keyboard(machine);
}



/*-------------------------------------------------
    ui_mess_get_use_natural_keyboard - returns
	whether the natural keyboard is active
-------------------------------------------------*/

int ui_mess_get_use_natural_keyboard(running_machine *machine)
{
	return machine->ui_mess_data->use_natural_keyboard;
}



/*-------------------------------------------------
    ui_mess_set_use_natural_keyboard - specifies
	whether the natural keyboard is active
-------------------------------------------------*/

void ui_mess_set_use_natural_keyboard(running_machine *machine, int use_natural_keyboard)
{
	machine->ui_mess_data->use_natural_keyboard = use_natural_keyboard;
}



/*-------------------------------------------------
    ui_mess_menu_keyboard_mode - menu that 	
-------------------------------------------------*/

void ui_mess_menu_keyboard_mode(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *event;
	int natural = ui_mess_get_use_natural_keyboard(machine);
	
	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
	{
		ui_menu_item_append(menu, "Keyboard Mode:", natural ? "Natural" : "Emulated", natural ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, NULL);
	}

	/* process the menu */
	event = ui_menu_process(machine, menu, 0);

	if (event != NULL)
	{
		if (event->iptkey == IPT_UI_LEFT || event->iptkey == IPT_UI_RIGHT) {
			ui_mess_set_use_natural_keyboard(machine, natural ^ TRUE);
			ui_menu_reset(menu, UI_MENU_RESET_REMEMBER_REF);
		}
	}		
}



/*-------------------------------------------------
    ui_mess_menu_keyboard_mode - populate MESS-specific menus 	
-------------------------------------------------*/

void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu)
{
	const input_field_config *field;
	const input_port_config *port;
	int has_keyboard = FALSE;

	/* scan the input port array to see what options we need to enable */
	for (port = machine->portlist.head; port != NULL; port = port->next)
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (field->type == IPT_KEYBOARD)
				has_keyboard = TRUE;			
		}
	}

  	/* add image info menu */
	ui_menu_item_append(menu, "Image Information", NULL, 0, ui_mess_menu_image_info);

  	/* add image info menu */
	ui_menu_item_append(menu, "File Manager", NULL, 0, ui_mess_menu_file_manager);

#if HAS_WAVE
  	/* add tape control menu */
	if (device_list_first(&machine->config->devicelist, CASSETTE))
		ui_menu_item_append(menu, "Tape Control", NULL, 0, ui_mess_menu_tape_control);
#endif /* HAS_WAVE */

  	/* add keyboard mode menu */
  	if (has_keyboard && inputx_can_post(machine))
		ui_menu_item_append(menu, "Keyboard Mode", NULL, 0, ui_mess_menu_keyboard_mode);
}
