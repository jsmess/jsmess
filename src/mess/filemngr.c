/*********************************************************************

	filemngr.c

	MESS's clunky built-in file manager

	TODO
		- Support image creation arguments
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

#define ITEMREF_NEW_IMAGE_NAME	((void *) 0x0001)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* menu item type in the file selector */
enum _file_selector_entry_type
{
	SELECTOR_ENTRY_TYPE_EMPTY,
	SELECTOR_ENTRY_TYPE_CREATE,
	SELECTOR_ENTRY_TYPE_DRIVE,
	SELECTOR_ENTRY_TYPE_DIRECTORY,
	SELECTOR_ENTRY_TYPE_FILE
};
typedef enum _file_selector_entry_type file_selector_entry_type;



/* an entry within the file manager */
typedef struct _file_selector_entry file_selector_entry;
struct _file_selector_entry
{
	file_selector_entry *next;

	file_selector_entry_type type;
	const char *basename;
	const char *fullpath;
};



/* state of the file manager */
typedef struct _file_manager_menu_state file_manager_menu_state;
struct _file_manager_menu_state
{
	const device_config *selected_device;
	astring *current_directory;
	astring *current_file;
};



/* state of the file selector menu */
typedef struct _file_selector_menu_state file_selector_menu_state;
struct _file_selector_menu_state
{
	file_manager_menu_state *manager_menustate;
	file_selector_entry *entrylist;
};



/* state of the file creator menu */
typedef struct _file_create_menu_state file_create_menu_state;
struct _file_create_menu_state
{
	file_manager_menu_state *manager_menustate;
	char filename_buffer[1024];
};



/***************************************************************************
    MENU HELPERS
***************************************************************************/

/*-------------------------------------------------
    code_to_ascii - converts an input_code to its
	ASCII equivalent
-------------------------------------------------*/

static char code_to_ascii(input_code code)
{
	/* code, lower case (w/o shift), upper case (with shift), control */
	static const struct
	{
		input_code code;
		char ch;
		char shift_ch;
		char ctrl_ch;
	} code_to_char_table[] =
	{
		{ KEYCODE_0, '0', ')', 0},
		{ KEYCODE_1, '1', '!', 0},
		{ KEYCODE_2, '2', '"', 0},
		{ KEYCODE_3, '3', '#', 0},
		{ KEYCODE_4, '4', '$', 0},
		{ KEYCODE_5, '5', '%', 0},
		{ KEYCODE_6, '6', '^', 0},
		{ KEYCODE_7, '7', '&', 0},
		{ KEYCODE_8, '8', '*', 0},
		{ KEYCODE_9, '9', '(', 0},
		{ KEYCODE_A, 'a', 'A', 1},
		{ KEYCODE_B, 'b', 'B', 2},
		{ KEYCODE_C, 'c', 'C', 3},
		{ KEYCODE_D, 'd', 'D', 4},
		{ KEYCODE_E, 'e', 'E', 5},
		{ KEYCODE_F, 'f', 'F', 6},
		{ KEYCODE_G, 'g', 'G', 7},
		{ KEYCODE_H, 'h', 'H', 8},
		{ KEYCODE_I, 'i', 'I', 9},
		{ KEYCODE_J, 'j', 'J', 10},
		{ KEYCODE_K, 'k', 'K', 11},
		{ KEYCODE_L, 'l', 'L', 12},
		{ KEYCODE_M, 'm', 'M', 13},
		{ KEYCODE_N, 'n', 'N', 14},
		{ KEYCODE_O, 'o', 'O', 15},
		{ KEYCODE_P, 'p', 'P', 16},
		{ KEYCODE_Q, 'q', 'Q', 17},
		{ KEYCODE_R, 'r', 'R', 18},
		{ KEYCODE_S, 's', 'S', 19},
		{ KEYCODE_T, 't', 'T', 20},
		{ KEYCODE_U, 'u', 'U', 21},
		{ KEYCODE_V, 'v', 'V', 22},
		{ KEYCODE_W, 'w', 'W', 23},
		{ KEYCODE_X, 'x', 'X', 24},
		{ KEYCODE_Y, 'y', 'Y', 25},
		{ KEYCODE_Z, 'z', 'Z', 26},
		{ KEYCODE_OPENBRACE, '[', '{', 27},
		{ KEYCODE_BACKSLASH, '\\', '|', 28},
		{ KEYCODE_CLOSEBRACE, ']', '}', 29},
		{ KEYCODE_TILDE, '^', '~', 30},
		{ KEYCODE_BACKSPACE, 127, 127, 31},
		{ KEYCODE_COLON, ':', ';', 0},
		{ KEYCODE_EQUALS, '=', '+', 0},
		{ KEYCODE_MINUS, '-', '_', 0},
		{ KEYCODE_STOP, '.', '<', 0},
		{ KEYCODE_COMMA, ',', '>', 0},
		{ KEYCODE_SLASH, '/', '?', 0},
		{ KEYCODE_ENTER, 13, 13, 13},
		{ KEYCODE_ESC, 27, 27, 27 }
	};

	int i;
	char result = 0;

	for (i = 0; i < ARRAY_LENGTH(code_to_char_table); i++)
	{
		if (code_to_char_table[i].code == code)
		{
			if (input_code_pressed(KEYCODE_LCONTROL) || input_code_pressed(KEYCODE_RCONTROL))
				result = code_to_char_table[i].ctrl_ch;
			else if (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT))
				result = code_to_char_table[i].shift_ch;
			else
				result = code_to_char_table[i].ch;
			break;
		}
	}
	return result;
}



/*-------------------------------------------------
    poll_keyboard - polls the keyboard and appends
	the result to the specified buffer
-------------------------------------------------*/

static void poll_keyboard(char *buffer, size_t buffer_length, int (*filter)(unicode_char))
{
	input_code code;
	char ascii_char;
	int length;

	/* poll keyboard */
	code = input_code_poll_switches(FALSE);
	if (code != INPUT_CODE_INVALID)
	{
		ascii_char = code_to_ascii(code);

		switch (ascii_char)
		{
			case 0:		/* NUL */
			case 13:	/* return */
			case 27:	/* escape */
				break;

			case 25:	/* Ctrl-Y (clear line) */
				buffer[0] = '\0';
				break;

			case 127:	/* delete */
				length = strlen(buffer);
				if (length > 0)
					buffer[length - 1] = '\0';
				break;

			default:
				if ((filter == NULL) || (*filter)(ascii_char))
				{
					/* got a char - add to string */
					snprintf(buffer + strlen(buffer), buffer_length - strlen(buffer), "%c", ascii_char);
				}
				break;
		}
	}
}



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

static void extra_text_render(running_machine *machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom,
	float origx1, float origy1, float origx2, float origy2,
	const char *header, const char *footer)
{
	header = ((header != NULL) && (header[0] != '\0')) ? header : NULL;
	footer = ((footer != NULL) && (footer[0] != '\0')) ? footer : NULL;

	if (header != NULL)
		extra_text_draw_box(origx1, origx2, origy1, top, header, -1);
	if (footer != NULL)
		extra_text_draw_box(origx1, origx2, origy2, bottom, footer, +1);
}



/***************************************************************************
    FILE CREATE MENU
***************************************************************************/

/*-------------------------------------------------
    is_valid_filename_char - tests to see if a
	character is valid in a filename 
-------------------------------------------------*/

static int is_valid_filename_char(unicode_char ch)
{
	/* this should really be in the OSD layer */
	static const char valid_filename_char[] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 00-0f */
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 10-1f */
		1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 	/*	!"#$%&'()*+,-./ */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 	/* 0123456789:;<=>? */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* @ABCDEFGHIJKLMNO */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 	/* PQRSTUVWXYZ[\]^_ */
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* `abcdefghijklmno */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 	/* pqrstuvwxyz{|}~	*/
	};
	return (ch < ARRAY_LENGTH(valid_filename_char)) && valid_filename_char[ch];
}



/*-------------------------------------------------
    file_creator_render_extra - perform our
    special rendering
-------------------------------------------------*/

static void file_creator_render_extra(running_machine *machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	file_create_menu_state *menustate = (file_create_menu_state *) state;

	extra_text_render(machine, menu, state, selectedref, top, bottom, origx1, origy1, origx2, origy2,
		astring_c(menustate->manager_menustate->current_directory),
		NULL);
}



/*-------------------------------------------------
    menu_file_create - file creator menu
-------------------------------------------------*/

static void menu_file_create(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	astring *new_path;
	const ui_menu_event *event;
	int underscore_pos = -1;
	file_create_menu_state *menustate = (file_create_menu_state *) state;

	/* clear the menu */
	ui_menu_reset(menu, 0);
	
	/* add the "[empty slot]" entry */
	ui_menu_item_append(menu, "New Image Name:", menustate->filename_buffer, 0, ITEMREF_NEW_IMAGE_NAME);

	/* set up custom render proc */
	ui_menu_set_custom_render(menu, file_creator_render_extra, ui_get_line_height() + 3.0f * UI_BOX_TB_BORDER, 0);

	/* features that are only enabled when selecting the file name */
	if (ui_menu_get_selection(menu) == 0)
	{
		/* poll the keyboard */
		poll_keyboard(menustate->filename_buffer, ARRAY_LENGTH(menustate->filename_buffer), is_valid_filename_char);

		/* put the underscore in the menu */
		underscore_pos = strlen(menustate->filename_buffer);
		snprintf(menustate->filename_buffer + underscore_pos, ARRAY_LENGTH(menustate->filename_buffer) - underscore_pos, "_");
	}

	/* process the menu */
	event = ui_menu_process(menu, 0);

	/* remove the underscore, if present */
	if (underscore_pos >= 0)
		menustate->filename_buffer[underscore_pos] = '\0';

	/* process the event */
	if (event != NULL && event->itemref != NULL)
	{
		/* handle selections */
		if (event->iptkey == IPT_UI_SELECT)
		{
			if (event->itemref == ITEMREF_NEW_IMAGE_NAME)
			{
				/* create the image */
				new_path = zippath_combine(
					astring_alloc(),
					astring_c(menustate->manager_menustate->current_directory), 
					menustate->filename_buffer);
				image_create(
					menustate->manager_menustate->selected_device,
					astring_c(new_path),
					0,
					NULL);
				astring_free(new_path);
			}
		}
	}
}



/***************************************************************************
    FILE SELECTOR MENU
***************************************************************************/

/*-------------------------------------------------
    file_selector_render_extra - perform our
    special rendering
-------------------------------------------------*/

static void file_selector_render_extra(running_machine *machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	file_selector_menu_state *menustate = (file_selector_menu_state *) state;

	extra_text_render(machine, menu, state, selectedref, top, bottom,
		origx1, origy1, origx2, origy2,
		astring_c(menustate->manager_menustate->current_directory),
		NULL);
}



/*-------------------------------------------------
    compare_file_selector_entries - sorting proc
	for file selector entries
-------------------------------------------------*/

static int compare_file_selector_entries(const file_selector_entry *e1, const file_selector_entry *e2)
{
	int result;
	const char *e1_basename = (e1->basename != NULL) ? e1->basename : "";
	const char *e2_basename = (e2->basename != NULL) ? e2->basename : "";

	if (e1->type < e2->type)
	{
		result = -1;
	}
	else if (e1->type > e2->type)
	{
		result = 1;
	}
	else
	{
		result = mame_stricmp(e1_basename, e2_basename);
		if (result == 0)
		{
			result = strcmp(e1_basename, e2_basename);
			if (result == 0)
			{
				if (e1 < e2)
					result = -1;
				else if (e1 > e2)
					result = 1;
			}
		}
	}

	return result;
}



/*-------------------------------------------------
    append_file_selector_entry - appends a new
	file selector entry to an entry list
-------------------------------------------------*/

static file_selector_entry *append_file_selector_entry(ui_menu *menu, file_selector_menu_state *menustate,
	file_selector_entry_type entry_type, const char *entry_basename, const char *entry_fullpath)
{
	file_selector_entry *entry;
	file_selector_entry **entryptr;

	/* allocate a new entry */
	entry = (file_selector_entry *) ui_menu_pool_alloc(menu, sizeof(*entry));
	memset(entry, 0, sizeof(*entry));
	entry->type = entry_type;
	entry->basename = (entry_basename != NULL) ? ui_menu_pool_strdup(menu, entry_basename) : entry_basename;
	entry->fullpath = (entry_fullpath != NULL) ? ui_menu_pool_strdup(menu, entry_fullpath) : entry_fullpath;

	/* find the end of the list */
	entryptr = &menustate->entrylist;
	while ((*entryptr != NULL) && (compare_file_selector_entries(entry, *entryptr) >= 0))
		entryptr = &(*entryptr)->next;

	/* insert the entry */
	entry->next = *entryptr;
	*entryptr = entry;
	return entry;
}



/*-------------------------------------------------
    append_file_selector_entry_menu_item - appends
	a menu item for a file selector entry
-------------------------------------------------*/

static file_selector_entry *append_dirent_file_selector_entry(ui_menu *menu, file_selector_menu_state *menustate,
	const osd_directory_entry *dirent)
{
	astring *buffer;
	file_selector_entry_type entry_type;
	file_selector_entry *entry;

	switch(dirent->type)
	{
		case ENTTYPE_FILE:
			entry_type = SELECTOR_ENTRY_TYPE_FILE;
			break;

		case ENTTYPE_DIR:
			entry_type = SELECTOR_ENTRY_TYPE_DIRECTORY;
			break;

		default:
			/* exceptional case; do not add a menu item */
			return NULL;
	}

	/* determine the full path */
	buffer = zippath_combine(
		astring_alloc(),
		astring_c(menustate->manager_menustate->current_directory),
		dirent->name);

	/* create the file selector entry */
	entry = append_file_selector_entry(
		menu,
		menustate,
		entry_type,
		dirent->name,
		astring_c(buffer));

	astring_free(buffer);
	return entry;
}



/*-------------------------------------------------
    append_file_selector_entry_menu_item - appends
	a menu item for a file selector entry
-------------------------------------------------*/

static void append_file_selector_entry_menu_item(ui_menu *menu, const file_selector_entry *entry)
{
	const char *text = NULL;
	const char *subtext = NULL;

	switch(entry->type)
	{
		case SELECTOR_ENTRY_TYPE_EMPTY:
			text = "[empty slot]";
			break;

		case SELECTOR_ENTRY_TYPE_CREATE:
			text = "[create]";
			break;

		case SELECTOR_ENTRY_TYPE_DRIVE:
			text = entry->basename;
			subtext = "[DRIVE]";
			break;

		case SELECTOR_ENTRY_TYPE_DIRECTORY:
			text = entry->basename;
			subtext = "[DIR]";
			break;

		case SELECTOR_ENTRY_TYPE_FILE:
			text = entry->basename;
			subtext = "[FILE]";
			break;
	}
	ui_menu_item_append(menu, text, subtext, 0, (void *) entry);
}



/*-------------------------------------------------
    menu_file_selector_populate - creates and
	allocates all menu items for a directory
-------------------------------------------------*/

static file_error menu_file_selector_populate(running_machine *machine, ui_menu *menu, file_selector_menu_state *menustate)
{
	zippath_directory *directory = NULL;
	file_error err = FILERR_NONE;
	const osd_directory_entry *dirent;
	const file_selector_entry *entry;
	const file_selector_entry *selected_entry = NULL;
	int count, i;
	image_device_info info;
	const device_config *device = menustate->manager_menustate->selected_device;
	const char *path = astring_c(menustate->manager_menustate->current_directory);

	/* open the directory */
	err = zippath_opendir(path, &directory);
	if (err != FILERR_NONE)
		goto done;

	/* clear out the menu entries */
	menustate->entrylist = NULL;

	/* add the "[empty slot]" entry */
	append_file_selector_entry(menu, menustate, SELECTOR_ENTRY_TYPE_EMPTY, NULL, NULL);

	info = image_device_getinfo(device->machine->config, device);
	if (info.creatable && !zippath_is_zip(directory))
	{
		/* add the "[create]" entry */
		append_file_selector_entry(menu, menustate, SELECTOR_ENTRY_TYPE_CREATE, NULL, NULL);
	}

	/* add the drives */
	count = osd_num_devices();
	for (i = 0; i < count; i++)
	{
		append_file_selector_entry(menu, menustate, SELECTOR_ENTRY_TYPE_DRIVE,
			osd_get_device_name(i), osd_get_device_name(i));
	}

	/* build the menu for each item */
	while((dirent = zippath_readdir(directory)) != NULL)
	{
		/* append a dirent entry */
		entry = append_dirent_file_selector_entry(menu, menustate, dirent);

		if (entry != NULL)
		{
			/* set the selected item to be the first non-parent directory or file */
			if ((selected_entry == NULL) && strcmp(dirent->name, ".."))
				selected_entry = entry;

			/* do we have to select this file? */
			if (!mame_stricmp(astring_c(menustate->manager_menustate->current_file), dirent->name))
				selected_entry = entry;
		}
	}

	/* append all of the menu entries */
	for (entry = menustate->entrylist; entry != NULL; entry = entry->next)
		append_file_selector_entry_menu_item(menu, entry);

	/* set the selection (if we have one) */
	if (selected_entry != NULL)
		ui_menu_set_selection(menu, (void *) selected_entry);

	/* set up custom render proc */
	ui_menu_set_custom_render(menu, file_selector_render_extra, ui_get_line_height() + 3.0f * UI_BOX_TB_BORDER, 0);

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

static void menu_file_selector(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	file_error err;
	const ui_menu_event *event;
	ui_menu *child_menu;
	file_selector_menu_state *menustate;
	file_create_menu_state *child_menustate;
	const file_selector_entry *entry;

	/* get menu state */
	menustate = (file_selector_menu_state *) state;

	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
	{
		err = menu_file_selector_populate(machine, menu, menustate);

		/* pop out if there was an error */
		if (err != FILERR_NONE)
		{
			ui_menu_stack_pop(machine);
			return;
		}
	}

	/* process the menu */
	event = ui_menu_process(menu, 0);
	if (event != NULL && event->itemref != NULL)
	{
		/* handle selections */
		if (event->iptkey == IPT_UI_SELECT)
		{
			entry = (const file_selector_entry *) event->itemref;
			switch(entry->type)
			{
				case SELECTOR_ENTRY_TYPE_EMPTY:
					/* empty slot - unload */
					image_unload(menustate->manager_menustate->selected_device);
					ui_menu_stack_pop(machine);
					break;

				case SELECTOR_ENTRY_TYPE_CREATE:
					/* create */
					child_menu = ui_menu_alloc(machine, menu_file_create, NULL);
					child_menustate = ui_menu_alloc_state(child_menu, sizeof(*child_menustate));
					child_menustate->manager_menustate = menustate->manager_menustate;
					ui_menu_stack_push(child_menu);
					break;

				case SELECTOR_ENTRY_TYPE_DRIVE:
				case SELECTOR_ENTRY_TYPE_DIRECTORY:
					/* drive/directory - first check the path */
					err = check_path(entry->fullpath);
					if (err != FILERR_NONE)
					{
						/* this path is problematic; present the user with an error and bail */
						ui_popup_time(1, "Error accessing %s", entry->fullpath);
						break;
					}
					astring_cpyc(menustate->manager_menustate->current_directory, entry->fullpath);
					ui_menu_reset(menu, 0);
					break;

				case SELECTOR_ENTRY_TYPE_FILE:
					/* file */
					image_load(menustate->manager_menustate->selected_device, entry->fullpath);
					ui_menu_stack_pop(machine);
					break;
			}
		}
	}
}



/***************************************************************************
    FILE MANAGER
***************************************************************************/

/*-------------------------------------------------
    fix_working_directory - checks the working
	directory for this device to ensure that it
	"makes sense"
-------------------------------------------------*/

static void fix_working_directory(const device_config *device)
{
	/* if the image exists, set the working directory to the parent directory */
	if (image_exists(device))
	{
		astring *astr = astring_alloc();
		zippath_parent(astr, image_filename(device));
		image_set_working_directory(device, astring_c(astr));
		astring_free(astr);
	}

	/* check to see if the path exists; if not clear it */
	if (check_path(image_working_directory(device)) != FILERR_NONE)
		image_set_working_directory(device, NULL);
}



/*-------------------------------------------------
    file_manager_render_extra - perform our
    special rendering
-------------------------------------------------*/

static void file_manager_render_extra(running_machine *machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
//	const char *path = (selected_device != NULL) ? image_filename(selected_device) : NULL;
//	extra_text_render(machine, menu, state, selectedref, top, bottom,
//		origx1, origy1, origx2, origy2, NULL, path);
}



/*-------------------------------------------------
    menu_file_manager_populate - populates the main
	file manager menu
-------------------------------------------------*/

static void menu_file_manager_populate(running_machine *machine, ui_menu *menu, void *state)
{
	char buffer[2048];
	const device_config *device;
	const char *entry_basename;

	/* cycle through all devices for this system */
	for (device = image_device_first(machine->config); device != NULL; device = image_device_next(device))
	{
		/* get the image type/id */
		snprintf(buffer, ARRAY_LENGTH(buffer),
			"%s",
			image_typename_id(device));

		/* get the base name */
		entry_basename = image_basename(device);

		/* record the menu item */
		ui_menu_item_append(menu, buffer, (entry_basename != NULL) ? entry_basename : "---", 0, (void *) device);
	}

	/* set up custom render proc */
	ui_menu_set_custom_render(menu, file_manager_render_extra, 0, ui_get_line_height() + 3.0f * UI_BOX_TB_BORDER);
}



/*-------------------------------------------------
    menu_file_manager - main file manager menu
-------------------------------------------------*/

void menu_file_manager(running_machine *machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *event;
	file_manager_menu_state *menustate;
	ui_menu *child_menu;
	file_selector_menu_state *child_menustate;

	/* if no state, allocate now */
	if (state == NULL)
		state = ui_menu_alloc_state(menu, sizeof(*menustate));
	menustate = (file_manager_menu_state *) state;

	/* possible cleanups from the file selector - ugly global variable usage */
	menustate->selected_device = NULL;
	if (menustate->current_directory != NULL)
	{
		astring_free(menustate->current_directory);
		menustate->current_directory = NULL;
	}
	if (menustate->current_file != NULL)
	{
		astring_free(menustate->current_file);
		menustate->current_file = NULL;
	}

	/* if the menu isn't built, populate now */
	if (!ui_menu_populated(menu))
		menu_file_manager_populate(machine, menu, state);

	/* process the menu */
	event = ui_menu_process(menu, 0);
	if (event != NULL && event->iptkey == IPT_UI_SELECT)
	{
		menustate->selected_device = (const device_config *) event->itemref;
		if (menustate->selected_device != NULL)
		{
			/* ensure that the working directory for this device exists */
			fix_working_directory(menustate->selected_device);

			/* set up current_directory and current_file - depends on whether we have an image */
			menustate->current_directory = astring_cpyc(astring_alloc(), image_working_directory(menustate->selected_device));
			menustate->current_file = astring_cpyc(astring_alloc(),
				image_exists(menustate->selected_device) ? image_basename(menustate->selected_device) : "");

			/* reset the existing menu */
			ui_menu_reset(menu, 0);

			/* push the menu */
			child_menu = ui_menu_alloc(machine, menu_file_selector, NULL);
			child_menustate = ui_menu_alloc_state(child_menu, sizeof(*child_menustate));
			child_menustate->manager_menustate = menustate;
			ui_menu_stack_push(child_menu);
		}
	}
}
