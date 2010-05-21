/*********************************************************************

    uimess.c

    MESS supplement to ui.c.

*********************************************************************/

#include "emu.h"
#include "uimenu.h"
#include "uimess.h"
#include "uiinput.h"
#include "input.h"
#include "softlist.h"
#include "devices/cassette.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/


/*-------------------------------------------------
    ui_mess_handler_ingame - function to determine
    if the builtin UI should be disabled
-------------------------------------------------*/

void ui_mess_handler_ingame(running_machine *machine)
{
	running_device *dev;

	/* run display routine for devices */
	if (mame_get_phase(machine) == MAME_PHASE_RUNNING)
	{

		for (dev = machine->devicelist.first(); dev != NULL; dev = dev->next)
		{
			if (is_image_device(dev))
			{
				device_display_func display = (device_display_func) dev->get_config_fct(DEVINFO_FCT_DISPLAY);
				if (display != NULL)
				{
					(*display)(dev);
				}
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
	running_device *img;

	astring_printf(string, "%s\n\n", machine->gamedrv->description);

#if 0
	if (mess_ram_size > 0)
	{
		char buf2[RAM_STRING_BUFLEN];
		astring_catprintf(string, "RAM: %s\n\n", ram_string(buf2, mess_ram_size));
	}
#endif

	for (img = machine->devicelist.first(); img != NULL; img = img->next)
	{
		if (is_image_device(img))
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
    ui_mess_main_menu_populate - populate MESS-specific menus
-------------------------------------------------*/

void ui_mess_main_menu_populate(running_machine *machine, ui_menu *menu)
{
	/* add image info menu */
	ui_menu_item_append(menu, "Image Information", NULL, 0, (void*)ui_mess_menu_image_info);

	/* add file manager menu */
	ui_menu_item_append(menu, "File Manager", NULL, 0, (void*)ui_mess_menu_file_manager);

	/* add software menu */
	ui_menu_item_append(menu, "Software", NULL, 0, (void*)ui_mess_menu_software);

	/* add tape control menu */
	if (machine->devicelist.first(CASSETTE))
		ui_menu_item_append(menu, "Tape Control", NULL, 0, (void*)ui_mess_menu_tape_control);
}
