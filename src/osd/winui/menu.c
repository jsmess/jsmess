//============================================================
//
//  menu.c - Win32 MESS menus handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <winuser.h>
#include <ctype.h>
#include <tchar.h>

// MAME/MESS headers
#include "emu.h"
#include "emuopts.h"
#include "menu.h"
#include "ui.h"
#include "messres.h"
#include "windows/video.h"
#include "windows/input.h"
#include "dialog.h"
#include "opcntrl.h"
#include "strconv.h"
#include "debug/debugcpu.h"
#include "inptport.h"
#include "imagedev/cassette.h"
#include "windows/window.h"
#include "winutf8.h"

//============================================================
//  IMPORTS
//============================================================

// from timer.c
extern void win_timer_enable(int enabled);


//============================================================
//  PARAMETERS
//============================================================

#define ID_FRAMESKIP_0				10000
#define ID_DEVICE_0					11000
#define ID_JOYSTICK_0				12000
#define ID_INPUT_0					13000
#define ID_VIDEO_VIEW_0				14000

#define MAX_JOYSTICKS				(8)

enum
{
	DEVOPTION_OPEN,
	DEVOPTION_CREATE,
	DEVOPTION_CLOSE,
	DEVOPTION_CASSETTE_PLAYRECORD,
	DEVOPTION_CASSETTE_STOPPAUSE,
	DEVOPTION_CASSETTE_PLAY,
	DEVOPTION_CASSETTE_RECORD,
	DEVOPTION_CASSETTE_REWIND,
	DEVOPTION_CASSETTE_FASTFORWARD,
	DEVOPTION_MAX
};

#ifdef MAME_PROFILER
#define HAS_PROFILER	1
#else
#define HAS_PROFILER	0
#endif

//============================================================
//  LOCAL VARIABLES
//============================================================

static int use_input_categories;
static int joystick_menu_setup;
static char state_filename[MAX_PATH];

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions);



//============================================================
//  input_item_from_serial_number
//============================================================

static int input_item_from_serial_number(running_machine &machine, int serial_number,
	const input_port_config **port, const input_field_config **field, const input_setting_config **setting)
{
	int i;
	const input_port_config *this_port = NULL;
	const input_field_config *this_field = NULL;
	const input_setting_config *this_setting = NULL;

	i = 0;
	for (this_port = machine.m_portlist.first(); (i != serial_number) && (this_port != NULL); this_port = this_port->next())
	{
		i++;
		for (this_field = this_port->fieldlist; (i != serial_number) && (this_field != NULL); this_field = this_field->next)
		{
			i++;
			for (this_setting = this_field->settinglist; (i != serial_number) && (this_setting != NULL); this_setting = this_setting->next)
			{
				i++;
			}
		}
	}

	if (this_setting != NULL)
		this_field = this_setting->field;
	if (this_field != NULL)
		this_port = this_field->port;

	if (port != NULL)
		*port = this_port;
	if (field != NULL)
		*field = this_field;
	if (setting != NULL)
		*setting = this_setting;
	return (i == serial_number);
}



//============================================================
//  serial_number_from_input_item
//============================================================

static int serial_number_from_input_item(running_machine &machine, const input_port_config *port,
	const input_field_config *field, const input_setting_config *setting)
{
	int i;
	const input_port_config *this_port;
	const input_field_config *this_field;
	const input_setting_config *this_setting;

	i = 0;
	for (this_port = machine.m_portlist.first(); this_port != NULL; this_port = this_port->next())
	{
		if ((port == this_port) && (field == NULL) && (setting == NULL))
			return i;

		i++;
		for (this_field = this_port->fieldlist; this_field != NULL; this_field = this_field->next)
		{
			if ((port == this_port) && (field == this_field) && (setting == NULL))
				return i;

			i++;
			for (this_setting = this_field->settinglist; this_setting != NULL; this_setting = this_setting->next)
			{
				if ((port == this_port) && (field == this_field) && (setting == this_setting))
					return i;

				i++;
			}
		}
	}
	return -1;
}



//============================================================
//  customize_input
//============================================================

static void customize_input(running_machine &machine, HWND wnd, const char *title, int player, int inputclass)
{
	dialog_box *dlg;
	const input_port_config *port;
	const input_field_config *field;
	int this_inputclass, this_player, portslot_count = 0, i;

	struct
	{
		const input_field_config *field;
	} portslots[256];

	/* create the dialog */
	dlg = win_dialog_init(title, NULL);
	if (!dlg)
		goto done;

	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			this_inputclass = input_classify_port(field);
			if (input_field_name(field) && (this_inputclass == inputclass))
			{
				/* most of the time, the player parameter is the player number
                 * but in the case of INPUT_CLASS_CATEGORIZED, it is the
                 * category */
				if (inputclass == INPUT_CLASS_CATEGORIZED)
					this_player = field->category;
				else
					this_player = input_player_number(field);

				if (this_player == player)
				{
					/* store this InputPort/RECT combo in our list.  we do not
                     * necessarily want to add it yet because we the INI might
                     * want to reorder the tab order */
					if (portslot_count < ARRAY_LENGTH(portslots))
					{
						portslots[portslot_count].field = field;
					}
					portslot_count++;
				}
			}
		}
	}

	/* finally add the portselects to the dialog */
	if (portslot_count > ARRAY_LENGTH(portslots))
		portslot_count = ARRAY_LENGTH(portslots);
	for (i = 0; i < portslot_count; i++)
	{
		if (portslots[i].field)
		{
			if (win_dialog_add_portselect(dlg, portslots[i].field))
				goto done;
		}
	}

	/* ...and now add OK/Cancel */
	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	/* ...and finally display the dialog */
	win_dialog_runmodal(machine, wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//  customize_joystick
//============================================================

static void customize_joystick(running_machine &machine, HWND wnd, int joystick_num)
{
	customize_input(machine, wnd, "Joysticks/Controllers", joystick_num, INPUT_CLASS_CONTROLLER);
}



//============================================================
//  customize_keyboard
//============================================================

static void customize_keyboard(running_machine &machine, HWND wnd)
{
	customize_input(machine, wnd, "Emulated Keyboard", 0, INPUT_CLASS_KEYBOARD);
}



//============================================================
//  customize_miscinput
//============================================================

static void customize_miscinput(running_machine &machine, HWND wnd)
{
	customize_input(machine, wnd, "Miscellaneous Input", 0, INPUT_CLASS_MISC);
}



//============================================================
//  customize_categorizedinput
//============================================================

static void customize_categorizedinput(running_machine &machine, HWND wnd, int category)
{
	assert(category > 0);
	customize_input(machine, wnd, "Input", category, INPUT_CLASS_CATEGORIZED);
}



//============================================================
//  storeval_inputport
//============================================================

static void storeval_inputport(void *param, int val)
{
	const input_field_config *field = (input_field_config *) param;
	input_field_user_settings settings;

	input_field_get_user_settings(field, &settings);
	settings.value = (UINT16) val;
	input_field_set_user_settings(field, &settings);
}



//============================================================
//  customize_switches
//============================================================

static void customize_switches(running_machine &machine, HWND wnd, const char* title_string, UINT32 ipt_name)
{
	dialog_box *dlg;
	const input_port_config *port;
	const input_field_config *field;
	const input_setting_config *setting;
	const char *switch_name = NULL;
	input_field_user_settings settings;

	UINT32 type;

	dlg = win_dialog_init(title_string, NULL);
	if (!dlg)
		goto done;

	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			type = field->type;

			if (type == ipt_name)
			{
				switch_name = input_field_name(field);

				input_field_get_user_settings(field, &settings);

				if (win_dialog_add_combobox(dlg, switch_name, settings.value, storeval_inputport, (void *) field))
					goto done;

				for (setting = field->settinglist; setting != NULL; setting = setting->next)
				{
					if (win_dialog_add_combobox_item(dlg, setting->name, setting->value))
						goto done;
				}
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(machine, wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//  customize_dipswitches
//============================================================

static void customize_dipswitches(running_machine &machine, HWND wnd)
{
	customize_switches(machine, wnd, "Dip Switches", IPT_DIPSWITCH);
}



//============================================================
//  customize_configuration
//============================================================

static void customize_configuration(running_machine &machine, HWND wnd)
{
	customize_switches(machine, wnd, "Driver Configuration", IPT_CONFIG);
}



//============================================================
//  customize_analogcontrols
//============================================================

enum
{
	ANALOG_ITEM_KEYSPEED,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY
};



static void store_analogitem(void *param, int val, int selected_item)
{
	const input_field_config *field = (const input_field_config *) param;
	input_field_user_settings settings;

	input_field_get_user_settings(field, &settings);

	switch(selected_item)
	{
		case ANALOG_ITEM_KEYSPEED:		settings.delta = val;		break;
		case ANALOG_ITEM_CENTERSPEED:	settings.centerdelta = val;	break;
		case ANALOG_ITEM_REVERSE:		settings.reverse = val;		break;
		case ANALOG_ITEM_SENSITIVITY:	settings.sensitivity = val;	break;
	}
	input_field_set_user_settings(field, &settings);
}



static void store_delta(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_KEYSPEED);
}



static void store_centerdelta(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_CENTERSPEED);
}



static void store_reverse(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_REVERSE);
}



static void store_sensitivity(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_SENSITIVITY);
}



static int port_type_is_analog(int type)
{
	return (type >= __ipt_analog_start && type <= __ipt_analog_end);
}



static void customize_analogcontrols(running_machine &machine, HWND wnd)
{
	dialog_box *dlg;
	const input_port_config *port;
	const input_field_config *field;
	input_field_user_settings settings;
	const char *name;
	char buf[255];
	static const struct dialog_layout layout = { 120, 52 };

	dlg = win_dialog_init("Analog Controls", &layout);
	if (!dlg)
		goto done;

	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (port_type_is_analog(field->type))
			{
				input_field_get_user_settings(field, &settings);
				name = input_field_name(field);

				_snprintf(buf, ARRAY_LENGTH(buf),
					"%s %s", name, "Digital Speed");
				if (win_dialog_add_adjuster(dlg, buf, settings.delta, 1, 255, FALSE, store_delta, (void *) field))
					goto done;

				_snprintf(buf, ARRAY_LENGTH(buf),
					"%s %s", name, "Autocenter Speed");
				if (win_dialog_add_adjuster(dlg, buf, settings.centerdelta, 1, 255, FALSE, store_centerdelta, (void *) field))
					goto done;

				_snprintf(buf, ARRAY_LENGTH(buf),
					"%s %s", name, "Reverse");
				if (win_dialog_add_combobox(dlg, buf, settings.reverse ? 1 : 0, store_reverse, (void *) field))
					goto done;
				if (win_dialog_add_combobox_item(dlg, "Off", 0))
					goto done;
				if (win_dialog_add_combobox_item(dlg, "On", 1))
					goto done;

				_snprintf(buf, ARRAY_LENGTH(buf),
					"%s %s", name, "Sensitivity");
				if (win_dialog_add_adjuster(dlg, buf, settings.sensitivity, 1, 255, TRUE, store_sensitivity, (void *) field))
					goto done;
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(machine, wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}


//============================================================
//  win_dirname
//============================================================

static char *win_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = (char*)malloc(strlen(filename) + 1);
	if (!dirname)
		return NULL;

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}


//============================================================
//  state_dialog
//============================================================

static void state_dialog(HWND wnd, win_file_dialog_type dlgtype,
	DWORD fileproc_flags, bool is_load,
	running_machine &machine)
{
	win_open_file_name ofn;
	char *dir = NULL;
	int result = 0;

	if (state_filename[0])
	{
		dir = win_dirname(state_filename);
	}

	memset(&ofn, 0, sizeof(ofn));
	ofn.type = dlgtype;
	ofn.owner = wnd;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | fileproc_flags;
	ofn.filter = "State Files (*.sta)|*.sta|All Files (*.*)|*.*";
	ofn.initial_directory = dir;

	if (!core_filename_ends_with(ofn.filename, "sta"))
		snprintf(ofn.filename, ARRAY_LENGTH(ofn.filename), "%s.sta", state_filename);
	else
		snprintf(ofn.filename, ARRAY_LENGTH(ofn.filename), "%s", state_filename);

	result = win_get_file_name_dialog(&ofn);
	if (result)
	{
		// the core doesn't add the extension if it's an absolute path
		if (osd_is_absolute_path(ofn.filename) && !core_filename_ends_with(ofn.filename, "sta"))
			snprintf(state_filename, ARRAY_LENGTH(state_filename), "%s.sta", ofn.filename);
		else
			snprintf(state_filename, ARRAY_LENGTH(state_filename), "%s", ofn.filename);

		if (is_load) {
			machine.schedule_load(state_filename);
		} else {
			machine.schedule_save(state_filename);
		}
	}
	if (dir)
		free(dir);
}



static void state_load(HWND wnd, running_machine &machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_OPEN, OFN_FILEMUSTEXIST, TRUE, machine);
}

static void state_save_as(HWND wnd, running_machine &machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_SAVE, OFN_OVERWRITEPROMPT, FALSE, machine);
}

static void state_save(running_machine &machine)
{
	machine.schedule_save(state_filename);
}



//============================================================
//  format_combo_changed
//============================================================

struct file_dialog_params
{
	device_image_interface *dev;
	int *create_format;
	option_resolution **create_args;
};

static void format_combo_changed(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *changed_param)
{
	HWND wnd;
	int format_combo_val;
	device_image_interface *dev;
	const option_guide *guide;
	const char *optspec;
	struct file_dialog_params *params;
	int has_option;
	TCHAR t_buf1[128];
	char *utf8_buf1;

	params = (struct file_dialog_params *) changed_param;

	// locate the format control
	format_combo_val = notification ? (((OFNOTIFY *) notification)->lpOFN->nFilterIndex - 1) : 0;
	if (format_combo_val < 0)
		format_combo_val = 0;
	*(params->create_format) = format_combo_val;

	// compute our parameters
	dev = params->dev;
	guide = dev->device_get_creation_option_guide();
	optspec =dev->device_get_indexed_creatable_format(format_combo_val)->m_optspec;

	// set the default extension
	CommDlg_OpenSave_SetDefExt(GetParent(dlgwnd),
		(const char*)dev->device_get_indexed_creatable_format(format_combo_val)->m_extensions);

	// enumerate through all of the child windows
	wnd = NULL;
	while((wnd = FindWindowEx(dlgwnd, wnd, NULL, NULL)) != NULL)
	{
		// get label text, removing trailing NULL
		GetWindowText(wnd, t_buf1, ARRAY_LENGTH(t_buf1));
		utf8_buf1 = utf8_from_tstring(t_buf1);
		assert(utf8_buf1[strlen(utf8_buf1)-1] == ':');
		utf8_buf1[strlen(utf8_buf1)-1] = '\0';

		// find guide entry
		while(guide->option_type && strcmp(utf8_buf1, guide->display_name))
			guide++;

		wnd = GetNextWindow(wnd, GW_HWNDNEXT);
		if (wnd && guide)
		{
			// we now have the handle to the window, and the guide entry
			has_option = option_resolution_contains(optspec, guide->parameter);

			SendMessage(wnd, CB_GETLBTEXT, SendMessage(wnd, CB_GETCURSEL, 0, 0), (LPARAM) t_buf1);
			SendMessage(wnd, CB_RESETCONTENT, 0, 0);

			win_prepare_option_control(wnd,
				has_option ? guide : NULL,
				has_option ? optspec : NULL);
		}
		osd_free(utf8_buf1);
	}
}



//============================================================
//  storeval_option_resolution
//============================================================

struct storeval_optres_params
{
	struct file_dialog_params *fdparams;
	const option_guide *guide_entry;
};

static void storeval_option_resolution(void *storeval_param, int val)
{
	option_resolution *resolution;
	struct storeval_optres_params *params;
	device_image_interface *dev;
	char buf[16];

	params = (struct storeval_optres_params *) storeval_param;
	dev = params->fdparams->dev;

	// create the resolution, if necessary
	resolution = *(params->fdparams->create_args);
	if (!resolution)
	{
		const option_guide *optguide = dev->device_get_creation_option_guide();
		const image_device_format *format = dev->device_get_indexed_creatable_format(*(params->fdparams->create_format));

		resolution = option_resolution_create(optguide, format->m_optspec);
		if (!resolution)
			return;
		*(params->fdparams->create_args) = resolution;
	}

	snprintf(buf, ARRAY_LENGTH(buf), "%d", val);
	option_resolution_add_param(resolution, params->guide_entry->identifier, buf);
}



//============================================================
//  build_option_dialog
//============================================================

static dialog_box *build_option_dialog(device_image_interface *dev, char *filter, size_t filter_len, int *create_format, option_resolution **create_args)
{
	dialog_box *dialog;
	const option_guide *guide_entry;
	int found, pos;
	char buf[256];
	struct file_dialog_params *params;
	struct storeval_optres_params *storeval_params;
	static const struct dialog_layout filedialog_layout = { 44, 220 };
	const image_device_format *format;

	// make the filter
	pos = 0;
	for (format = dev->device_get_creatable_formats(); format != NULL; format = format->m_next)
	{
		pos += add_filter_entry(filter + pos, filter_len - pos, format->m_description, format->m_extensions);
	}

	// create the dialog
	dialog = win_dialog_init(NULL, &filedialog_layout);
	if (!dialog)
		goto error;

	// allocate the params
	params = (struct file_dialog_params *) win_dialog_malloc(dialog, sizeof(*params));
	if (!params)
		goto error;
	params->dev = dev;
	params->create_format = create_format;
	params->create_args = create_args;

	// set the notify handler; so that we get notified when the format dialog changed
	if (win_dialog_add_notification(dialog, CDN_TYPECHANGE, format_combo_changed, params))
		goto error;

	// loop through the entries
	for (guide_entry = dev->device_get_creation_option_guide(); guide_entry->option_type != OPTIONTYPE_END; guide_entry++)
	{
		// make sure that this entry is present on at least one option specification
		found = FALSE;
		for (format = dev->device_get_creatable_formats(); format != NULL; format = format->m_next)
		{
			if (option_resolution_contains(format->m_optspec, guide_entry->parameter))
			{
				found = TRUE;
				break;
			}
		}

		if (found)
		{
			storeval_params = (struct storeval_optres_params *) win_dialog_malloc(dialog, sizeof(*storeval_params));
			if (!storeval_params)
				goto error;
			storeval_params->fdparams = params;
			storeval_params->guide_entry = guide_entry;

			// this option is present on at least one of the specs
			switch(guide_entry->option_type)
			{
				case OPTIONTYPE_INT:
					snprintf(buf, ARRAY_LENGTH(buf), "%s:", guide_entry->display_name);
					if (win_dialog_add_combobox(dialog, buf, 0, storeval_option_resolution, storeval_params))
						goto error;
					break;

				default:
					break;
			}
		}
	}

	return dialog;

error:
	if (dialog)
		win_dialog_exit(dialog);
	return NULL;
}



//============================================================
//  copy_extension_list
//============================================================

static int copy_extension_list(char *dest, size_t dest_len, const char *extensions)
{
	const char *s;
	int pos = 0;

	// our extension lists are comma delimited; Win32 expects to see lists
	// delimited by semicolons
	s = extensions;
	while(*s)
	{
		// append a semicolon if not at the beginning
		if (s != extensions)
			pos += snprintf(&dest[pos], dest_len - pos, ";");

		// append ".*"
		pos += snprintf(&dest[pos], dest_len - pos, "*.");

		// append the file extension
		while(*s && (*s != ','))
		{
			pos += snprintf(&dest[pos], dest_len - pos, "%c", *s);
			s++;
		}

		// if we found a comma, advance
		while(*s == ',')
			s++;
	}
	return pos;
}



//============================================================
//  add_filter_entry
//============================================================

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions)
{
	int pos = 0;

	// add the description
	pos += snprintf(&dest[pos], dest_len - pos, "%s (", description);

	// add the extensions to the description
	pos += copy_extension_list(&dest[pos], dest_len - pos, extensions);

	// add the trailing rparen and '|' character
	pos += snprintf(&dest[pos], dest_len - pos, ")|");

	// now add the extension list itself
	pos += copy_extension_list(&dest[pos], dest_len - pos, extensions);

	// append a '|'
	pos += snprintf(&dest[pos], dest_len - pos, "|");

	return pos;
}



//============================================================
//  build_generic_filter
//============================================================

static void build_generic_filter(device_t *device, int is_save, char *filter, size_t filter_len)
{
	char *s;

	const char *file_extension;

	/* copy the string */
	file_extension = downcast<const legacy_image_device_config_base *>(&device->baseconfig())->file_extensions();

	// start writing the filter
	s = filter;

	// common image types
	s += add_filter_entry(filter, filter_len, "Common image types", file_extension);

	// all files
	s += sprintf(s, "All files (*.*)|*.*|");

	// compressed
	if (!is_save)
		s += sprintf(s, "Compressed Images (*.zip)|*.zip|");

	*(s++) = '\0';
}



//============================================================
//  change_device
//============================================================

static void change_device(HWND wnd, device_image_interface *image, int is_save)
{
	dialog_box *dialog = NULL;
	char filter[2048];
	char filename[MAX_PATH];
	const char *initial_dir;
	BOOL result;
	int create_format = 0;
	option_resolution *create_args = NULL;

	// sanity check
	assert(image != NULL);

	// get the file
	if (image->exists())
	{
		const char *imgname;
		imgname = image->basename();
		snprintf(filename, ARRAY_LENGTH(filename), "%s", imgname);
	}
	else
	{
		filename[0] = '\0';
	}

	// use image directory, if it is there
	initial_dir = image->working_directory();

	// add custom dialog elements, if appropriate
	if (is_save
		&& (image->device_get_creation_option_guide() != NULL)
		&& (image->device_get_creatable_formats() != NULL))
	{
		dialog = build_option_dialog(image, filter, ARRAY_LENGTH(filter), &create_format, &create_args);
		if (!dialog)
			goto done;
	}
	else
	{
		// build a normal filter
		build_generic_filter(&image->device(), is_save, filter, ARRAY_LENGTH(filter));
	}

	// display the dialog
	result = win_file_dialog(image->device().machine(), wnd, is_save ? WIN_FILE_DIALOG_SAVE : WIN_FILE_DIALOG_OPEN,
		dialog, filter, initial_dir, filename, ARRAY_LENGTH(filename));
	if (result)
	{
		// mount the image
		if (is_save)
			(image_error_t)image->create(filename, image->device_get_indexed_creatable_format(create_format), create_args);
		else
			(image_error_t)image->load( filename);

		// no need to check the returnvalue and show a messagebox anymore.
		// a UI message will now be generated by the image code
	}

done:
	if (dialog != NULL)
		win_dialog_exit(dialog);
	if (create_args != NULL)
		option_resolution_close(create_args);
}



//============================================================
//  pause
//============================================================

static void pause(running_machine &machine)
{
	if (!winwindow_ui_is_paused(machine)) {
		machine.pause();
	} else {
		machine.resume();
	}
}



//============================================================
//  get_menu_item_string
//============================================================

static BOOL get_menu_item_string(HMENU menu, UINT item, BOOL by_position, HMENU *sub_menu, LPTSTR buffer, size_t buffer_len)
{
	MENUITEMINFO mii;

	// clear out results
	memset(buffer, '\0', sizeof(*buffer) * buffer_len);
	if (sub_menu)
		*sub_menu = NULL;

	// prepare MENUITEMINFO structure
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE | (sub_menu ? MIIM_SUBMENU : 0);
	mii.dwTypeData = buffer;
	mii.cch = buffer_len;

	// call GetMenuItemInfo()
	if (!GetMenuItemInfo(menu, item, by_position, &mii))
		return FALSE;

	// return results
	if (sub_menu)
		*sub_menu = mii.hSubMenu;
	if (mii.fType == MFT_SEPARATOR)
		_sntprintf(buffer, buffer_len, TEXT("-"));
	return TRUE;
}



//============================================================
//  find_sub_menu
//============================================================

static HMENU find_sub_menu(HMENU menu, const char *menutext, int create_sub_menu)
{
	int i;
	TCHAR buf[128];
	HMENU sub_menu;

	while(*menutext)
	{
		TCHAR *t_menutext = tstring_from_utf8(menutext);

		i = -1;
		do
		{
			if (!get_menu_item_string(menu, ++i, TRUE, &sub_menu, buf, ARRAY_LENGTH(buf)))
			{
				osd_free(t_menutext);
				return NULL;
			}
		}
		while(_tcscmp(t_menutext, buf));

		osd_free(t_menutext);

		if (!sub_menu && create_sub_menu)
		{
			MENUITEMINFO mii;
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_SUBMENU;
			mii.hSubMenu = CreateMenu();
			if (!SetMenuItemInfo(menu, i, TRUE, &mii))
			{
				i = GetLastError();
				return NULL;
			}
			sub_menu = mii.hSubMenu;
		}
		menu = sub_menu;
		if (!menu)
			return NULL;
		menutext += strlen(menutext) + 1;
	}
	return menu;
}



//============================================================
//  set_command_state
//============================================================

static void set_command_state(HMENU menu_bar, UINT command, UINT state)
{
	BOOL result;

	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	result = SetMenuItemInfo(menu_bar, command, FALSE, &mii);
}




//============================================================
//  remove_menu_items
//============================================================

static void remove_menu_items(HMENU menu)
{
	while(RemoveMenu(menu, 0, MF_BYPOSITION))
		;
}



//============================================================
//  setup_joystick_menu
//============================================================

static void setup_joystick_menu(running_machine &machine, HMENU menu_bar)
{
	int joystick_count = 0;
	HMENU joystick_menu;
	int i;
	UINT command;
	HMENU submenu = NULL;
	const input_port_config *port;
	const input_field_config *field;
	const input_setting_config *setting;
	char buf[256];
	int child_count = 0;

	use_input_categories = 0;
	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (field->type == IPT_CATEGORY)
			{
				use_input_categories = 1;
				break;
			}
		}
	}

	joystick_menu = find_sub_menu(menu_bar, "&Options\0&Joysticks\0", TRUE);
	if (!joystick_menu)
		return;

	if (use_input_categories)
	{
		// using input categories
		for (port = machine.m_portlist.first(); port != NULL; port = port->next())
		{
			for (field = port->fieldlist; field != NULL; field = field->next)
			{
				if (field->type == IPT_CATEGORY)
				{
					submenu = CreateMenu();
					if (!submenu)
						return;

					// append all of the category settings
					for (setting = field->settinglist; setting != NULL; setting = setting->next)
					{
						command = ID_INPUT_0 + serial_number_from_input_item(machine, port, field, setting);
						win_append_menu_utf8(submenu, MF_STRING, command, setting->name);
					}

					// tack on the final items and the menu item
					command = ID_INPUT_0 + serial_number_from_input_item(machine, port, field, NULL);
					AppendMenu(submenu, MF_SEPARATOR, 0, NULL);
					AppendMenu(submenu, MF_STRING, command, TEXT("&Configure..."));
					win_append_menu_utf8(joystick_menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, field->name);
					child_count++;
				}
			}
		}
	}
	else
	{
		// set up joystick menu
		joystick_count = input_count_players(machine);
		if (joystick_count > 0)
		{
			for (i = 0; i < joystick_count; i++)
			{
				snprintf(buf, ARRAY_LENGTH(buf), "Joystick %i", i + 1);
				win_append_menu_utf8(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, buf);
				child_count++;
			}
		}
	}

	// last but not least, enable the joystick menu (or not)
	set_command_state(menu_bar, ID_OPTIONS_JOYSTICKS, child_count ? MFS_ENABLED : MFS_GRAYED);
}



//============================================================
//  is_windowed
//============================================================

static int is_windowed(void)
{
	return video_config.windowed;
}



//============================================================
//  frameskip_level_count
//============================================================

static int frameskip_level_count(running_machine &machine)
{
	static int count = -1;

	if (count < 0)
	{
		//int frameskip_min = 0;
		int frameskip_max = 10;
		//options_get_range_int(&machine.options(), OPTION_FRAMESKIP, &frameskip_min, &frameskip_max);
		count = frameskip_max + 1;
	}
	return count;
}



//============================================================
//  prepare_menus
//============================================================

static void prepare_menus(HWND wnd)
{
	int i;
	char buf[MAX_PATH];
	TCHAR t_buf[MAX_PATH];
	const char *s;
	HMENU menu_bar;
	HMENU video_menu;
	HMENU device_menu;
	HMENU sub_menu;
	UINT_PTR new_item;
	UINT flags_for_exists;
	UINT flags_for_writing;
	device_image_interface *img = NULL;
	int has_config, has_dipswitch, has_keyboard, has_analog, has_misc;
	const input_port_config *port;
	const input_field_config *field;
	const input_setting_config *setting;
	int frameskip;
	int orientation;
	int speed;
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	const char *view_name;
	int view_index;
	UINT command;
	input_field_user_settings settings;

	menu_bar = GetMenu(wnd);
	if (!menu_bar)
		return;

	if (!joystick_menu_setup)
	{
		setup_joystick_menu(window->machine(), menu_bar);
		joystick_menu_setup = 1;
	}

	frameskip = window->machine().video().frameskip();

	orientation = window->target->orientation();

	speed = window->machine().video().throttled() ? window->machine().video().speed_factor() : 0;

	has_config		= input_has_input_class(window->machine(), INPUT_CLASS_CONFIG);
	has_dipswitch	= input_has_input_class(window->machine(), INPUT_CLASS_DIPSWITCH);
	has_keyboard	= input_has_input_class(window->machine(), INPUT_CLASS_KEYBOARD);
	has_misc		= input_has_input_class(window->machine(), INPUT_CLASS_MISC);

	has_analog = 0;
	for (port = window->machine().m_portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (port_type_is_analog(field->type))
			{
				has_analog = 1;
				break;
			}
		}
	}

	set_command_state(menu_bar, ID_FILE_SAVESTATE,			state_filename[0] != '\0'					? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_EDIT_PASTE,				inputx_can_post(window->machine())			? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_OPTIONS_PAUSE,			winwindow_ui_is_paused(window->machine())		? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_CONFIGURATION,	has_config									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_DIPSWITCHES,		has_dipswitch								? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_MISCINPUT,		has_misc									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_ANALOGCONTROLS,	has_analog									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_FULLSCREEN,		!is_windowed()								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_TOGGLEFPS,		ui_get_show_fps()							? MFS_CHECKED : MFS_ENABLED);
#if HAS_PROFILER
	set_command_state(menu_bar, ID_OPTIONS_PROFILER,		ui_get_show_profiler()						? MFS_CHECKED : MFS_ENABLED);
#endif

	set_command_state(menu_bar, ID_KEYBOARD_EMULATED,		(has_keyboard) ?
																(!ui_get_use_natural_keyboard(window->machine())					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_NATURAL,		(has_keyboard && inputx_can_post(window->machine())) ?																(ui_get_use_natural_keyboard(window->machine())					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_CUSTOMIZE,		has_keyboard								? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_VIDEO_ROTATE_0,			(orientation == ROT0)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_90,			(orientation == ROT90)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_180,		(orientation == ROT180)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_270,		(orientation == ROT270)						? MFS_CHECKED : MFS_ENABLED);

	set_command_state(menu_bar, ID_THROTTLE_50,				(speed == 50)								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_100,			(speed == 100)								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_200,			(speed == 200)								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_500,			(speed == 500)								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_1000,			(speed == 1000)								? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_UNTHROTTLED,	(speed == 0)								? MFS_CHECKED : MFS_ENABLED);

	set_command_state(menu_bar, ID_FRAMESKIP_AUTO,			(frameskip < 0)								? MFS_CHECKED : MFS_ENABLED);
	for (i = 0; i < frameskip_level_count(window->machine()); i++)
		set_command_state(menu_bar, ID_FRAMESKIP_0 + i,		(frameskip == i)							? MFS_CHECKED : MFS_ENABLED);

	// if we are using categorized input, we need to properly checkmark the categories
	if (use_input_categories)
	{
		for (port = window->machine().m_portlist.first(); port != NULL; port = port->next())
		{
			for (field = port->fieldlist; field != NULL; field = field->next)
			{
				if (field->type == IPT_CATEGORY)
				{
					input_field_get_user_settings(field, &settings);
					for (setting = field->settinglist; setting != NULL; setting = setting->next)
					{
						command = ID_INPUT_0 + serial_number_from_input_item(window->machine(), port, field, setting);
						set_command_state(menu_bar, command, (setting->value == settings.value) ? MFS_CHECKED : MFS_ENABLED);
					}
				}
			}
		}
	}

	// set up screens in video menu
	video_menu = find_sub_menu(menu_bar, "&Options\0&Video\0", FALSE);
	do
	{
		get_menu_item_string(video_menu, 0, TRUE, NULL, t_buf, ARRAY_LENGTH(t_buf));
		if (_tcscmp(t_buf, TEXT("-")))
			RemoveMenu(video_menu, 0, MF_BYPOSITION);
	}
	while(_tcscmp(t_buf, TEXT("-")));
	i = 0;
	view_index = window->target->view();
	while((view_name = window->target->view_name(i)) != NULL)
	{
		TCHAR *t_view_name = tstring_from_utf8(view_name);
		InsertMenu(video_menu, i, MF_BYPOSITION | (i == view_index ? MF_CHECKED : 0),
			ID_VIDEO_VIEW_0 + i, t_view_name);
		osd_free(t_view_name);
		i++;
	}

	// set up device menu; first remove all existing menu items
	device_menu = find_sub_menu(menu_bar, "&Devices\0", FALSE);
	remove_menu_items(device_menu);

	int cnt = 0;
	// then set up the actual devices
	for (bool gotone = window->machine().m_devicelist.first(img); gotone; gotone = img->next(img))
	{
		new_item = ID_DEVICE_0 + (cnt * DEVOPTION_MAX);
		flags_for_exists = MF_STRING;

		if (!img->exists())
			flags_for_exists |= MF_GRAYED;

		flags_for_writing = flags_for_exists;
		if (!img->is_writable())
			flags_for_writing |= MF_GRAYED;

		sub_menu = CreateMenu();
		win_append_menu_utf8(sub_menu, MF_STRING,		new_item + DEVOPTION_OPEN,		"Mount...");

		if (img->image_config().is_creatable())
			win_append_menu_utf8(sub_menu, MF_STRING,	new_item + DEVOPTION_CREATE,	"Create...");

		win_append_menu_utf8(sub_menu, flags_for_exists,	new_item + DEVOPTION_CLOSE,	"Unmount");

		if (img->device().type() == CASSETTE)
		{
			cassette_state state;
			state = (cassette_state)(img->exists() ? (cassette_get_state(&img->device()) & CASSETTE_MASK_UISTATE) : CASSETTE_STOPPED);
			win_append_menu_utf8(sub_menu, MF_SEPARATOR, 0, NULL);
			win_append_menu_utf8(sub_menu, flags_for_exists	| ((state == CASSETTE_STOPPED)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_STOPPAUSE,	"Pause/Stop");
			win_append_menu_utf8(sub_menu, flags_for_exists	| ((state == CASSETTE_PLAY)		? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_PLAY,			"Play");
			win_append_menu_utf8(sub_menu, flags_for_writing	| ((state == CASSETTE_RECORD)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_RECORD,		"Record");
			win_append_menu_utf8(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_REWIND,		"Rewind");
			win_append_menu_utf8(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_FASTFORWARD,	"Fast Forward");
		}
		s = img->exists() ? img->filename() : "[empty slot]";

		snprintf(buf, ARRAY_LENGTH(buf), "%s: %s", img->image_config().devconfig().name(), s);
		win_append_menu_utf8(device_menu, MF_POPUP, (UINT_PTR)sub_menu, buf);

		cnt++;
	}
}



//============================================================
//  set_seped
//============================================================

static void set_speed(running_machine &machine, int speed)
{
	astring error_string;
	if (speed != 0)
	{
		machine.video().set_speed_factor(speed);
		machine.options().emu_options::set_value(OPTION_SPEED, speed / 100, OPTION_PRIORITY_CMDLINE, error_string);
	}

	machine.video().set_throttled(speed != 0);
	machine.options().emu_options::set_value(OPTION_THROTTLE, (speed != 0), OPTION_PRIORITY_CMDLINE, error_string);
}



//============================================================
//  win_toggle_menubar
//============================================================

void win_toggle_menubar(void)
{
	win_window_info *window;
	LONG width_diff;
	LONG height_diff;
	DWORD style, exstyle;
	HWND hwnd;
	HMENU menu;

	for (window = win_window_list; window != NULL; window = window->next)
	{
		RECT before_rect = { 100, 100, 200, 200 };
		RECT after_rect = { 100, 100, 200, 200 };

		hwnd = window->hwnd;

		// get current menu
		menu = GetMenu(hwnd);

		// get before rect
		style = GetWindowLong(hwnd, GWL_STYLE);
		exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		AdjustWindowRectEx(&before_rect, style, menu ? TRUE : FALSE, exstyle);

		// toggle the menu
		if (menu)
		{
			SetProp(hwnd, TEXT("menu"), (HANDLE) menu);
			menu = NULL;
		}
		else
		{
			menu = (HMENU) GetProp(hwnd, TEXT("menu"));
		}
		SetMenu(hwnd, menu);

		// get after rect, and width/height diff
		AdjustWindowRectEx(&after_rect, style, menu ? TRUE : FALSE, exstyle);
		width_diff = (after_rect.right - after_rect.left) - (before_rect.right - before_rect.left);
		height_diff = (after_rect.bottom - after_rect.top) - (before_rect.bottom - before_rect.top);

		if (is_windowed())
		{
			RECT window_rect;
			GetWindowRect(hwnd, &window_rect);
			SetWindowPos(hwnd, HWND_TOP, 0, 0,
				window_rect.right - window_rect.left + width_diff,
				window_rect.bottom - window_rect.top + height_diff,
				SWP_NOMOVE | SWP_NOZORDER);
		}

		RedrawWindow(hwnd, NULL, NULL, 0);
	}
}



//============================================================
//  device_command
//============================================================

static void device_command(HWND wnd, device_image_interface *img, int devoption)
{
	switch(devoption)
	{
		case DEVOPTION_OPEN:
			change_device(wnd, img, FALSE);
			break;

		case DEVOPTION_CREATE:
			change_device(wnd, img, TRUE);
			break;

		case DEVOPTION_CLOSE:
			img->unload();
			break;

		default:
			if (img->device().type() == CASSETTE)
			{
				switch(devoption)
				{
					case DEVOPTION_CASSETTE_STOPPAUSE:
						cassette_change_state(&img->device(), CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_PLAY:
						cassette_change_state(&img->device(), CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_RECORD:
						cassette_change_state(&img->device(), CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_REWIND:
						cassette_seek(&img->device(), -60.0, SEEK_CUR);
						break;

					case DEVOPTION_CASSETTE_FASTFORWARD:
						cassette_seek(&img->device(), +60.0, SEEK_CUR);
						break;
				}
			}
			break;
	}
}



//============================================================
//  help_display
//============================================================

static void help_display(HWND wnd, const char *chapter)
{
	typedef HWND (WINAPI *htmlhelpproc)(HWND hwndCaller, LPCTSTR pszFile, UINT uCommand, DWORD_PTR dwData);
	static htmlhelpproc htmlhelp;
	static DWORD htmlhelp_cookie;
	LPCSTR htmlhelp_funcname;

	if (htmlhelp == NULL)
	{
#ifdef UNICODE
		htmlhelp_funcname = "HtmlHelpW";
#else
		htmlhelp_funcname = "HtmlHelpA";
#endif
		htmlhelp = (htmlhelpproc) GetProcAddress(LoadLibrary(TEXT("hhctrl.ocx")), htmlhelp_funcname);
		if (!htmlhelp)
			return;
		htmlhelp(NULL, NULL, 28 /*HH_INITIALIZE*/, (DWORD_PTR) &htmlhelp_cookie);
	}

	// if full screen, turn it off
	if (!is_windowed())
		winwindow_toggle_full_screen();

	{
		TCHAR *t_chapter = tstring_from_utf8(chapter);
		htmlhelp(wnd, t_chapter, 0 /*HH_DISPLAY_TOPIC*/, 0);
		osd_free(t_chapter);
	}
}



//============================================================
//  help_about_mess
//============================================================

static void help_about_mess(HWND wnd)
{
	help_display(wnd, "mess.chm::/windows/main.htm");
}



//============================================================
//  help_about_thissystem
//============================================================

static void help_about_thissystem(running_machine &machine, HWND wnd)
{
	char buf[256];
	snprintf(buf, ARRAY_LENGTH(buf), "mess.chm::/sysinfo/%s.htm", machine.system().name);
	help_display(wnd, buf);
}



//============================================================
//  decode_deviceoption
//============================================================

static device_image_interface *decode_deviceoption(running_machine &machine, int command, int *devoption)
{
	int absolute_index;

	command -= ID_DEVICE_0;
	absolute_index = command / DEVOPTION_MAX;

	if (devoption)
		*devoption = command % DEVOPTION_MAX;

	return image_from_absolute_index(machine, absolute_index);
}



//============================================================
//  set_window_orientation
//============================================================

static void set_window_orientation(win_window_info *window, int orientation)
{
	window->target->set_orientation(orientation);
	if (window->target->is_ui_target())
	{
		render_container::user_settings settings;
		window->machine().render().ui_container().get_user_settings(settings);
		settings.m_orientation = orientation;
		window->machine().render().ui_container().set_user_settings(settings);
	}
	winwindow_video_window_update(window);
}



//============================================================
//  pause_for_command
//============================================================

static int pause_for_command(UINT command)
{
	// we really should be more conservative and only pause for commands
	// that do dialog stuff
	return (command != ID_OPTIONS_PAUSE);
}



//============================================================
//  invoke_command
//============================================================

static int invoke_command(HWND wnd, UINT command)
{
	astring error_string;
	int handled = 1;
	int dev_command;
	device_image_interface *img;
	UINT16 category;
	const input_field_config *field;
	const input_setting_config *setting;
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	input_field_user_settings settings;

	// pause while invoking certain commands
	if (pause_for_command(command))
		winwindow_ui_pause_from_window_thread(window->machine(), TRUE);

	switch(command)
	{
		case ID_FILE_LOADSTATE_NEWUI:
			state_load(wnd, window->machine());
			break;

		case ID_FILE_SAVESTATE:
			state_save(window->machine());
			break;

		case ID_FILE_SAVESTATE_AS:
			state_save_as(wnd, window->machine());
			break;

		case ID_FILE_SAVESCREENSHOT:
			window->machine().video().save_active_screen_snapshots();
			break;

		case ID_FILE_EXIT_NEWUI:
			window->machine().schedule_exit();
			break;

		case ID_EDIT_PASTE:
			PostMessage(wnd, WM_PASTE, 0, 0);
			break;

		case ID_KEYBOARD_NATURAL:
			ui_set_use_natural_keyboard(window->machine(), TRUE);
			break;

		case ID_KEYBOARD_EMULATED:
			ui_set_use_natural_keyboard(window->machine(), FALSE);
			break;

		case ID_KEYBOARD_CUSTOMIZE:
			customize_keyboard(window->machine(), wnd);
			break;

		case ID_VIDEO_ROTATE_0:
			set_window_orientation(window, ROT0);
			break;

		case ID_VIDEO_ROTATE_90:
			set_window_orientation(window, ROT90);
			break;

		case ID_VIDEO_ROTATE_180:
			set_window_orientation(window, ROT180);
			break;

		case ID_VIDEO_ROTATE_270:
			set_window_orientation(window, ROT270);
			break;

		case ID_OPTIONS_PAUSE:
			pause(window->machine());
			break;

		case ID_OPTIONS_HARDRESET:
			window->machine().schedule_hard_reset();
			break;

		case ID_OPTIONS_SOFTRESET:
			window->machine().schedule_soft_reset();
			break;

#if HAS_PROFILER
		case ID_OPTIONS_PROFILER:
			ui_set_show_profiler(!ui_get_show_profiler());
			break;
#endif // HAS_PROFILER

		case ID_OPTIONS_DEBUGGER:
			debug_cpu_get_visible_cpu(window->machine())->debug()->halt_on_next_instruction("User-initiated break\n");
			break;

		case ID_OPTIONS_CONFIGURATION:
			customize_configuration(window->machine(), wnd);
			break;

		case ID_OPTIONS_DIPSWITCHES:
			customize_dipswitches(window->machine(), wnd);
			break;

		case ID_OPTIONS_MISCINPUT:
			customize_miscinput(window->machine(), wnd);
			break;

		case ID_OPTIONS_ANALOGCONTROLS:
			customize_analogcontrols(window->machine(), wnd);
			break;

		case ID_OPTIONS_FULLSCREEN:
			winwindow_toggle_full_screen();
			break;

		case ID_OPTIONS_TOGGLEFPS:
			ui_set_show_fps(!ui_get_show_fps());
			break;

		case ID_OPTIONS_USEMOUSE:
			{
				// FIXME
//              extern int win_use_mouse;
//              win_use_mouse = !win_use_mouse;
			}
			break;

		case ID_OPTIONS_TOGGLEMENUBAR:
			win_toggle_menubar();
			break;

		case ID_FRAMESKIP_AUTO:
			window->machine().video().set_frameskip(-1);
			window->machine().options().emu_options::set_value(OPTION_AUTOFRAMESKIP, 1, OPTION_PRIORITY_CMDLINE, error_string);
			break;

		case ID_HELP_ABOUT_NEWUI:
			help_about_mess(wnd);
			break;

		case ID_HELP_ABOUTSYSTEM:
			help_about_thissystem(window->machine(), wnd);
			break;

		case ID_THROTTLE_50:
			set_speed(window->machine(), 50);
			break;

		case ID_THROTTLE_100:
			set_speed(window->machine(), 100);
			break;

		case ID_THROTTLE_200:
			set_speed(window->machine(), 200);
			break;

		case ID_THROTTLE_500:
			set_speed(window->machine(), 500);
			break;

		case ID_THROTTLE_1000:
			set_speed(window->machine(), 1000);
			break;

		case ID_THROTTLE_UNTHROTTLED:
			set_speed(window->machine(), 0);
			break;

		default:
			if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + frameskip_level_count(window->machine())))
			{
				// change frameskip
				window->machine().video().set_frameskip(command - ID_FRAMESKIP_0);
				window->machine().options().emu_options::set_value(OPTION_AUTOFRAMESKIP, 0, OPTION_PRIORITY_CMDLINE, error_string);
				window->machine().options().emu_options::set_value(OPTION_FRAMESKIP, (int)command - ID_FRAMESKIP_0, OPTION_PRIORITY_CMDLINE, error_string);
			}
			else if ((command >= ID_DEVICE_0) && (command < ID_DEVICE_0 + (IO_COUNT*DEVOPTION_MAX)))
			{
				// change device
				img = decode_deviceoption(window->machine(), command, &dev_command);
				device_command(wnd, img, dev_command);
			}
			else if ((command >= ID_JOYSTICK_0) && (command < ID_JOYSTICK_0 + MAX_JOYSTICKS))
			{
				// customize joystick
				customize_joystick(window->machine(), wnd, command - ID_JOYSTICK_0);
			}
			else if ((command >= ID_VIDEO_VIEW_0) && (command < ID_VIDEO_VIEW_0 + 1000))
			{
				// render views
				window->target->set_view(command - ID_VIDEO_VIEW_0);
			}
			else if (input_item_from_serial_number(window->machine(), command - ID_INPUT_0, NULL, &field, &setting))
			{
				if ((field != NULL) && (field->type == IPT_CATEGORY) && (setting != NULL))
				{
					// change the input type for this category
					input_field_get_user_settings(field, &settings);
					settings.value = setting->value;
					input_field_set_user_settings(field, &settings);
				}
				else if ((field != NULL) && (field->type == IPT_CATEGORY) && (setting == NULL))
				{
					// customize the input type
					input_field_get_user_settings(field, &settings);
					category = 0;

					for (setting = field->settinglist; setting != NULL; setting = setting->next)
					{
						if (settings.value == setting->value)
						{
							category = setting->category;
						}
					}
					if (category)
						customize_categorizedinput(window->machine(), wnd, category);
				}
				else
				{
					// should never happen
					handled = 0;
				}
			}
			else
			{
				// bogus command
				handled = 0;
			}
			break;
	}

	// resume emulation
	if (pause_for_command(command))
		winwindow_ui_pause_from_window_thread(window->machine(), FALSE);

	return handled;
}



//============================================================
//  set_menu_text
//============================================================

static void set_menu_text(HMENU menu_bar, int command, const char *text)
{
	TCHAR *t_text;
	MENUITEMINFO mii;

	// convert to TCHAR
	t_text = tstring_from_utf8(text);

	// invoke SetMenuItemInfo()
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = t_text;
	SetMenuItemInfo(menu_bar, command, FALSE, &mii);

	// cleanup
	osd_free(t_text);
}



//============================================================
//  win_setup_menus
//============================================================

int win_setup_menus(running_machine &machine, HMODULE module, HMENU menu_bar)
{
	HMENU frameskip_menu;
	char buf[256];
	int i;

	// verify that our magic numbers work
	assert((ID_DEVICE_0 + IO_COUNT * DEVOPTION_MAX) < ID_JOYSTICK_0);

	// initialize critical values
	joystick_menu_setup = 0;

	// remove the profiler menu item if it doesn't exist
#if HAS_PROFILER
	ui_set_show_profiler(0);
#else
	DeleteMenu(menu_bar, ID_OPTIONS_PROFILER, MF_BYCOMMAND);
#endif

	if ((machine.debug_flags & DEBUG_FLAG_ENABLED) == 0)
		DeleteMenu(menu_bar, ID_OPTIONS_DEBUGGER, MF_BYCOMMAND);

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0", FALSE);
	if (!frameskip_menu)
		return 1;
	for(i = 0; i < frameskip_level_count(machine); i++)
	{
		snprintf(buf, ARRAY_LENGTH(buf), "%i", i);
		win_append_menu_utf8(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf);
	}

	// set the help menu to refer to this machine
	snprintf(buf, ARRAY_LENGTH(buf), "About %s (%s)...", machine.system().description, machine.system().name);
	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, buf);

	// initialize state_filename for each driver, so we don't carry names in-between them
	{
		char *src;
		char *dst;

		snprintf(state_filename, ARRAY_LENGTH(state_filename),
			"%s State", machine.system().description);

		src = state_filename;
		dst = state_filename;
		do
		{
			if ((*src == '\0') || isalnum(*src) || isspace(*src) || strchr("(),.", *src))
				*(dst++) = *src;
		}
		while(*(src++));
	}

	return 0;
}



//============================================================
//  win_resource_module
//============================================================

static HMODULE win_resource_module(void)
{
	static HMODULE module;
	if (module == NULL)
	{
		MEMORY_BASIC_INFORMATION info;
		if ((VirtualQuery((const void*)win_resource_module, &info, sizeof(info))) == sizeof(info))
			module = (HMODULE)info.AllocationBase;
	}
	return module;
}



//============================================================
//  win_create_menu
//============================================================

int win_create_menu(running_machine &machine, HMENU *menus)
{
	HMENU menu_bar = NULL;
	HMODULE module;

	module = win_resource_module();
	menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
	if (!menu_bar)
		goto error;

	if (win_setup_menus(machine, module, menu_bar))
		goto error;

	*menus = menu_bar;
	return 0;

error:
	if (menu_bar)
		DestroyMenu(menu_bar);
	return 1;
}



//============================================================
//  winwindow_video_window_proc_ui
//============================================================

LRESULT CALLBACK winwindow_video_window_proc_ui(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
		case WM_INITMENU:
			prepare_menus(wnd);
			break;

		case WM_PASTE:
			{
				LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
				win_window_info *window = (win_window_info *)ptr;
				ui_paste(window->machine());
			}
			break;

		case WM_COMMAND:
			if (invoke_command(wnd, wparam))
				break;
			/* fall through */

		default:
			return winwindow_video_window_proc(wnd, message, wparam, lparam);
	}
	return 0;
}
