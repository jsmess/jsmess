/*********************************************************************

	filemngr.c

	MESS's clunky built-in file manager

	TODO
		- Support image creation; the old file manager did not do this
		  well.
			- Should only allow image creation if the device allows
			- Also support creation arguments
		- Restrict directory listing by file extension
		- Support file manager invocation from the main menu for
		  required images
		- Restrict empty slot if image required

*********************************************************************/

#include "driver.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "zippath.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SLOT_EMPTY		"[empty slot]"
#define SLOT_CREATE		"[create]"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef union _fileselector_state fileselector_state;
union _fileselector_state
{
	UINT32 i;
	struct
	{
		int selected : 16;
		unsigned int menu_is_built : 1;
	} s;
};



/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static const device_config *selected_device;
static astring *current_directory;
static astring *current_file;



/***************************************************************************
    MENU HELPERS
***************************************************************************/

/*-------------------------------------------------
    extra_text_draw_box - generically adds header
	or footer text
-------------------------------------------------*/

static void extra_text_draw_box(float origx1, float origx2, float origy, float yspan, const char *text, int direction)
{
	float width, maxwidth;
	float x1, y1, x2, y2, temp;

	/* get the size of the text */
	ui_draw_text_full(text, 0.0f, 0.0f, 1.0f, JUSTIFY_CENTER, WRAP_TRUNCATE,
		DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &width, NULL);
	width += 2 * UI_BOX_LR_BORDER;
	maxwidth = MAX(width, origx2 - origx1);

	/* compute our bounds */
	x1 = 0.5f - 0.5f * maxwidth;
	x2 = x1 + maxwidth;
	y1 = origy + (yspan * direction);
	y2 = origy + (UI_BOX_TB_BORDER * direction);

	if (y1 > y2)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	/* draw a box */
	ui_draw_outlined_box(x1, y1, x2, y2, UI_FILLCOLOR);

	/* take off the borders */
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	/* draw the text within it */
	ui_draw_text_full(text, x1, y1, x2 - x1, JUSTIFY_CENTER, WRAP_TRUNCATE,
					  DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}



/*-------------------------------------------------
    extra_text_render - generically adds header
	and footer text
-------------------------------------------------*/

static void extra_text_render(const menu_extra *extra, float origx1, float origy1, float origx2, float origy2,
	const char *header, const char *footer)
{
	header = ((header != NULL) && (header[0] != '\0')) ? header : NULL;
	footer = ((footer != NULL) && (footer[0] != '\0')) ? footer : NULL;

	if (header != NULL)
		extra_text_draw_box(origx1, origx2, origy1, extra->top, header, -1);
	if (footer != NULL)
		extra_text_draw_box(origx1, origx2, origy2, extra->bottom, footer, +1);
}



/***************************************************************************
    FILE CREATE MENU
***************************************************************************/

/*-------------------------------------------------
    menu_file_create - file creator menu
-------------------------------------------------*/

static UINT32 menu_file_create(running_machine *machine, UINT32 state)
{
	fatalerror("NYI");
	return 0;
}



/***************************************************************************
    FILE SELECTOR MENU
***************************************************************************/

/*-------------------------------------------------
    alloc_directory_entry - allocates a struct
	of type osd_directory_entry
-------------------------------------------------*/

static osd_directory_entry *alloc_directory_entry(const char *name, osd_dir_entry_type type, UINT64 size)
{
	char *name_dupe = NULL;
	osd_directory_entry *new_entry;
	size_t name_length;
	
	/* allocate the new entry */
	name_length = (name != NULL) ? strlen(name) + 1 : 0;
	new_entry = malloc_or_die(sizeof(*new_entry) + name_length);

	/* copy the name, if specified */
	if (name != NULL)
	{
		name_dupe = ((char *) new_entry) + sizeof(*new_entry);
		strcpy(name_dupe, name);
	}

	new_entry->name = name_dupe;
	new_entry->type = type;
	new_entry->size = size;
	return new_entry;
}



/*-------------------------------------------------
    dupe_directory_entry - duplicates a struct
	of type osd_directory_entry
-------------------------------------------------*/

static osd_directory_entry *dupe_directory_entry(const osd_directory_entry *entry)
{
	return alloc_directory_entry(entry->name, entry->type, entry->size);
}



/*-------------------------------------------------
    file_selector_render_extra - perform our
    special rendering
-------------------------------------------------*/

static void file_selector_render_extra(const menu_extra *extra, float origx1, float origy1, float origx2, float origy2)
{
	extra_text_render(extra, origx1, origy1, origx2, origy2, astring_c(current_directory), NULL);
}



/*-------------------------------------------------
    build_file_selector_menu_items - creates and
	allocates all menu items for a directory
-------------------------------------------------*/

static file_error build_file_selector_menu_items(const device_config *device, const char *path,
	ui_menu_item *item_list, size_t item_list_length, int *menu_items, int *selected)
{
	zippath_directory *directory = NULL;
	file_error err = FILERR_NONE;
	const osd_directory_entry *dirent;
	osd_directory_entry *dirent_dupe;
	const char *subtext;
	int count, i;
	image_device_info info;

	/* reset count */
	*menu_items = 0;
	*selected = 0;

	/* add the "[empty slot]" entry */
	memset(&item_list[*menu_items], 0, sizeof(item_list[*menu_items]));
	item_list[*menu_items].text = SLOT_EMPTY;
	item_list[*menu_items].ref = alloc_directory_entry(NULL, ENTTYPE_FILE, 0);
	(*menu_items)++;

	info = image_device_getinfo(device->machine->config, device);
	if (0 && info.creatable)	/* NYI */
	{
		/* add the "[create]" entry */
		memset(&item_list[*menu_items], 0, sizeof(item_list[*menu_items]));
		item_list[*menu_items].text = SLOT_CREATE;
		item_list[*menu_items].ref = alloc_directory_entry(NULL, ENTTYPE_FILE, 0);
		(*menu_items)++;
	}

	/* add the drives */
	count = osd_num_devices();
	for (i = 0; i < count; i++)
	{
		memset(&item_list[*menu_items], 0, sizeof(item_list[*menu_items]));
		item_list[*menu_items].text = osd_get_device_name(i);
		item_list[*menu_items].subtext = "[DRIVE]";
		item_list[*menu_items].ref = alloc_directory_entry(item_list[*menu_items].text, ENTTYPE_DIR, 0);
		(*menu_items)++;
	}

	/* open the directory */
	err = zippath_opendir(path, &directory);
	if (err != FILERR_NONE)
		goto done;

	/* build the menu for each item */
	while((dirent = zippath_readdir(directory)) != NULL)
	{
		/* if there are too many entries... */
		if (*menu_items >= item_list_length - 10)
			break;

		/* set the selected item to be the first non-parent directory or file */
		if ((*selected == 0) && strcmp(dirent->name, ".."))
			*selected = *menu_items;

		/* choose text for the entry type */
		switch(dirent->type)
		{
			case ENTTYPE_FILE:
				subtext = "[FILE]";
				break;
			case ENTTYPE_DIR:
				subtext = "[DIR]";
				break;
			default:
				subtext = "[UNK]";
				break;
		}

		/* dupe the menu item */
		dirent_dupe = dupe_directory_entry(dirent);

		/* do we have to select this file? */
		if (!mame_stricmp(astring_c(current_file), dirent_dupe->name))
			*selected = *menu_items;

		/* record the menu item */
		memset(&item_list[*menu_items], 0, sizeof(item_list[*menu_items]));
		item_list[*menu_items].text = dirent_dupe->name;
		item_list[*menu_items].subtext = subtext;
		item_list[*menu_items].ref = dirent_dupe;
		(*menu_items)++;
	}

	/* add an item for the return */
	memset(&item_list[*menu_items], 0, sizeof(item_list[*menu_items]));
	item_list[*menu_items].text = "Return to Prior Menu";
	(*menu_items)++;

done:
	if (directory != NULL)
		zippath_closedir(directory);
	return err;
}



/*-------------------------------------------------
    check_path - performs a quick check to see if
	a path exists
-------------------------------------------------*/

static file_error check_path(const char *path)
{
	return zippath_opendir(path, NULL);
}



/*-------------------------------------------------
    menu_file_selector - file selector menu
-------------------------------------------------*/

static UINT32 menu_file_selector(running_machine *machine, UINT32 state)
{
	static ui_menu_item item_list[200];
	static int menu_items;

	file_error err;
	const osd_directory_entry *dirent;
	int visible_items, selected, i;
	astring *new_path;
	fileselector_state fs_state;
	int menu_is_built;
	menu_extra extra;

	/* interpret the state */
	fs_state.i = state;
	selected = fs_state.s.selected;
	menu_is_built = fs_state.s.menu_is_built;

	/* do we have to build the menu? */
	if (!menu_is_built)
	{
		err = build_file_selector_menu_items(selected_device, astring_c(current_directory),
			item_list, ARRAY_LENGTH(item_list), &menu_items, &selected);
		if (err != FILERR_NONE)
		{
			/* if we error here, we must pop out */
			fs_state.i = ui_menu_stack_pop();
			goto done;
		}

		menu_is_built = TRUE;
	}

	/* compute extras */
	memset(&extra, 0, sizeof(extra));
	extra.top = ui_get_line_height() + 3.0f * UI_BOX_TB_BORDER;
	extra.render = file_selector_render_extra;

	/* draw the menu */
	visible_items = ui_menu_draw(item_list, menu_items, selected, &extra);

	/* handle the keys */
	if (ui_menu_generic_keys(machine, (UINT32 *) &selected, menu_items, visible_items))
		goto done;

	if (input_ui_pressed(machine, IPT_UI_SELECT))
	{
		dirent = (const osd_directory_entry *) item_list[selected].ref;
		menu_is_built = FALSE;

		switch(dirent->type)
		{
			case ENTTYPE_DIR:
				/* prepare the selection in the new directory */
				selected = 1;
				if (!strcmp(dirent->name, ".."))
					zippath_parent_basename(current_file, astring_c(current_directory));

				/* change the directory */
				new_path = zippath_combine(astring_alloc(), astring_c(current_directory), dirent->name);
				err = check_path(astring_c(new_path));
				if (err != FILERR_NONE)
				{
					/* this path is problematic; present the user with an error and bail */
					ui_popup_time(1, "Error accessing %s", astring_c(new_path));
					astring_free(new_path);
					break;
				}
				astring_free(current_directory);
				current_directory = new_path;
				break;

			case ENTTYPE_FILE:
				if (dirent->name != NULL)
				{
					/* specified a file */
					new_path = zippath_combine(astring_alloc(), astring_c(current_directory), dirent->name);
					image_load(selected_device, astring_c(new_path));
					astring_free(new_path);
				}
				else if (!strcmp(item_list[selected].text, SLOT_EMPTY))
				{
					/* empty slot - unload */
					image_unload(selected_device);
				}
				else if (!strcmp(item_list[selected].text, SLOT_CREATE))
				{
					/* create */
					fs_state.i = ui_menu_stack_push(menu_file_create, 0);
				}
				else
				{
					fatalerror("Should not reach here");
				}
				fs_state.i = ui_menu_stack_pop();
				goto done;

			default:
				/* do nothing */
				break;
		}
	}

	/* reassemble state */
	fs_state.s.selected = selected;
	fs_state.s.menu_is_built = menu_is_built;

done:
	if (!menu_is_built)
	{
		/* time to tear down the menus */
		for (i = 0; i < menu_items; i++)
		{
			if (item_list[i].ref != NULL)
			{
				free(item_list[i].ref);
				item_list[i].ref = NULL;
			}
		}
	}

	return fs_state.i;
}



/***************************************************************************
    FILE MANAGER
***************************************************************************/

/*-------------------------------------------------
    file_manager_render_extra - perform our
    special rendering
-------------------------------------------------*/

static void file_manager_render_extra(const menu_extra *extra, float origx1, float origy1, float origx2, float origy2)
{
	const char *path = (selected_device != NULL) ? image_filename(selected_device) : NULL;
	extra_text_render(extra, origx1, origy1, origx2, origy2, NULL, path);
}



/*-------------------------------------------------
    menu_file_manager - main file manager menu
-------------------------------------------------*/

UINT32 menu_file_manager(running_machine *machine, UINT32 state)
{
	const device_config *device;
	ui_menu_item item_list[40];
	int menu_items = 0;
	char buffer[2048];
	int buffer_pos = 0;
	const char *entry_typename;
	const char *entry_basename;
	int visible_items;
	int selected_absolute_index = -1;
	fileselector_state fs_state;
	menu_extra extra;

	/* possible cleanups from the file selector - ugly global variable usage */
	selected_device = NULL;
	if (current_directory != NULL)
	{
		astring_free(current_directory);
		current_directory = NULL;
	}
	if (current_file != NULL)
	{
		astring_free(current_file);
		current_file = NULL;
	}

	/* cycle through all devices for this system */
	for (device = image_device_first(machine->config); device != NULL; device = image_device_next(device))
	{
		/* sanity check */
		if ((buffer_pos >= ARRAY_LENGTH(buffer) || (menu_items >= ARRAY_LENGTH(item_list))))
			break;

		/* is this selected? */
		if (state == menu_items)
		{
			selected_device = device;
			selected_absolute_index = image_absolute_index(device);
		}

		/* get the image type/id */
		entry_typename = &buffer[buffer_pos];
		buffer_pos += snprintf(&buffer[buffer_pos],
			ARRAY_LENGTH(buffer) - buffer_pos,
			"%s",
			image_typename_id(device)) + 1;

		/* get the base name */
		entry_basename = image_basename(device);

		/* record the menu item */
		memset(&item_list[menu_items], 0, sizeof(item_list[menu_items]));
		item_list[menu_items].text = entry_typename;
		item_list[menu_items].subtext = (entry_basename != NULL) ? entry_basename : "---";
		menu_items++;
	}

	/* add an item for the return */
	memset(&item_list[menu_items], 0, sizeof(item_list[menu_items]));
	item_list[menu_items++].text = "Return to Prior Menu";

	/* compute extras */
	memset(&extra, 0, sizeof(extra));
	extra.bottom = ui_get_line_height() + 3.0f * UI_BOX_TB_BORDER;
	extra.render = file_manager_render_extra;

	/* draw the menu */
	visible_items = ui_menu_draw(item_list, menu_items, state, &extra);

	/* handle the keys */
	if (ui_menu_generic_keys(machine, &state, menu_items, visible_items))
		return state;

	/* was selected pressed? */
	if (input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if (selected_absolute_index >= 0)
		{
			/* set up current_directory and current_file - depends on whether we have an image */
			if (image_exists(selected_device))
			{
				/* note that we _should_ be able to use image_working_directory(); please fix */
				current_directory = zippath_parent(astring_alloc(), image_filename(selected_device));
				current_file = astring_cpyc(astring_alloc(), image_basename(selected_device));
			}
			else
			{
				current_directory = astring_cpyc(astring_alloc(), image_working_directory(selected_device));
				current_file = astring_cpyc(astring_alloc(), "");
			}

			memset(&fs_state, 0, sizeof(fs_state));
			return ui_menu_stack_push(menu_file_selector, fs_state.i);
		}
		else
		{
			return ui_menu_stack_pop();
		}
	}

	return state;
}
