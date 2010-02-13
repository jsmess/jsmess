//============================================================
//
//  menu.c - Win32 MESS menus handling
//
//============================================================

// standard windows headers
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
#include "utils.h"
#include "png.h"
#include "debug/debugcpu.h"
#include "inptport.h"
#include "devices/cassette.h"
#include "windows/window.h"
#include "uimess.h"
#include "winutf8.h"

#ifdef UNDER_CE
#include "invokegx.h"
#else
#include "configms.h"
#endif

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

#ifdef UNDER_CE
#define HAS_TOGGLEMENUBAR		0
#define HAS_TOGGLEFULLSCREEN	0
#else
#define HAS_TOGGLEMENUBAR		1
#define HAS_TOGGLEFULLSCREEN	1
#endif

#ifdef UNDER_CE
#define WM_INITMENU		WM_INITMENUPOPUP
#define MFS_GRAYED		MF_GRAYED
#define WMSZ_BOTTOM		6
#endif

//============================================================
//  LOCAL VARIABLES
//============================================================

static HICON device_icons[IO_COUNT];
static int use_input_categories;
static int joystick_menu_setup;
static char state_filename[MAX_PATH];

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions);

/***************************************************************************

    Constants

***************************************************************************/

typedef enum
{
	ARTWORK_CUSTTYPE_JOYSTICK = 0,
	ARTWORK_CUSTTYPE_KEYBOARD,
	ARTWORK_CUSTTYPE_MISC,
	ARTWORK_CUSTTYPE_INVALID
} artwork_cust_type;



/***************************************************************************

    Type definitions

***************************************************************************/

struct inputform_customization
{
	UINT32 ipt;
	int x, y;
	int width, height;
};


//============================================================
//  artwork_get_inputscreen_customizations
//============================================================

int artwork_get_inputscreen_customizations(png_info *png, artwork_cust_type cust_type,
	const char *section,
	struct inputform_customization *customizations,
	int customizations_length)
{
	file_error filerr;
	mame_file *file;
	char buffer[1000];
	char current_section[64];
	char ipt_name[64];
	char *p;
	int x1, y1, x2, y2;
	const char *png_filename;
	const char *ini_filename;
	int enabled = TRUE;
	int item_count = 0;

	static const char *const cust_files[] =
	{
		"ctrlr.png",		"ctrlr.ini",
		"keyboard.png",		"keyboard.ini"
		"misc.png",			"misc.ini"
	};

	if ((cust_type >= 0) && (cust_type < (ARRAY_LENGTH(cust_files) / 2)))
	{
		png_filename = cust_files[cust_type * 2 + 0];
		ini_filename = cust_files[cust_type * 2 + 1];
	}
	else
	{
		png_filename = NULL;
		ini_filename = NULL;
	}

	/* subtract one from the customizations length; so we can place IPT_END */
	customizations_length--;

	/* open the INI file, if available */
	if (ini_filename)
	{
		filerr = mame_fopen(SEARCHPATH_ARTWORK, ini_filename, OPEN_FLAG_READ, &file);
		if (filerr == FILERR_NONE)
		{
			/* loop until we run out of lines */
			while (customizations_length && mame_fgets(buffer, sizeof(buffer), file))
			{
				/* strip off any comments */
				p = strstr(buffer, "//");
				if (p)
					*p = 0;

				/* section header? */
				if (buffer[0] == '[')
				{
					strncpyz(current_section, &buffer[1],
						ARRAY_LENGTH(current_section));
					p = strchr(current_section, ']');
					if (!p)
						continue;
					*p = '\0';
					if (section)
						enabled = !mame_stricmp(current_section, section);
					continue;
				}

				if (!enabled || sscanf(buffer, "%64s (%d,%d)-(%d,%d)", ipt_name, &x1, &y1, &x2, &y2) != 5)
					continue;

#if 0
				/* temporarily disabled */
				for (pik = input_keywords; pik->name[0]; pik++)
				{
					pik_name = pik->name;
					if ((pik_name[0] == 'P') && (pik_name[1] == '1') && (pik_name[2] == '_'))
						pik_name += 3;

					if (!strcmp(ipt_name, pik_name))
					{
						if ((x1 > 0) && (y1 > 0) && (x2 > x1) && (y2 > y1))
						{
							customizations->ipt = pik->val;
							customizations->x = x1;
							customizations->y = y1;
							customizations->width = x2 - x1;
							customizations->height = y2 - y1;
							customizations++;
							customizations_length--;
							item_count++;
						}
						break;
					}
				}
#endif
			}
			mame_fclose(file);
		}
	}

	/* terminate list */
	customizations->ipt = IPT_END;
	customizations->x = -1;
	customizations->y = -1;
	customizations->width = -1;
	customizations->height = -1;

	/* open the PNG, if available */
	memset(png, 0, sizeof(*png));
	if (png_filename && item_count > 0)
	{
		filerr = mame_fopen(SEARCHPATH_ARTWORK, ini_filename, OPEN_FLAG_READ, &file);
		if (filerr == FILERR_NONE)
		{
			png_read_file(mame_core_file(file), png);
			mame_fclose(file);
		}
	}
	return item_count;
}


//============================================================
//  input_item_from_serial_number
//============================================================

static int input_item_from_serial_number(running_machine *machine, int serial_number,
	const input_port_config **port, const input_field_config **field, const input_setting_config **setting)
{
	int i;
	const input_port_config *this_port = NULL;
	const input_field_config *this_field = NULL;
	const input_setting_config *this_setting = NULL;

	i = 0;
	for (this_port = machine->portlist.first(); (i != serial_number) && (this_port != NULL); this_port = this_port->next)
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

static int serial_number_from_input_item(running_machine *machine, const input_port_config *port,
	const input_field_config *field, const input_setting_config *setting)
{
	int i;
	const input_port_config *this_port;
	const input_field_config *this_field;
	const input_setting_config *this_setting;

	i = 0;
	for (this_port = machine->portlist.first(); this_port != NULL; this_port = this_port->next)
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

static void customize_input(running_machine *machine, HWND wnd, const char *title, artwork_cust_type cust_type, int player, int inputclass, const char *section)
{
	dialog_box *dlg;
	const input_port_config *port;
	const input_field_config *field;
	png_info png;
	struct inputform_customization customizations[128];
	RECT *pr;
	int this_inputclass, this_player, portslot_count, i;

	struct
	{
		const input_field_config *field;
		const RECT *pr;
	} portslots[256];

	/* check to see if there is any custom artwork for this configure dialog,
     * and if so, retrieve the info */
	portslot_count = artwork_get_inputscreen_customizations(&png, cust_type, section,
		customizations, ARRAY_LENGTH(customizations));

	/* zero out the portslots that correspond to customizations */
	memset(portslots, 0, sizeof(portslots[0]) * portslot_count);

	/* create the dialog */
	dlg = win_dialog_init(title, NULL);
	if (!dlg)
		goto done;

	if (png.width > 0)
	{
		win_dialog_add_image(dlg, &png);
		win_dialog_add_separator(dlg);
	}

	for (port = machine->portlist.first(); port != NULL; port = port->next)
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
					/* check to see if the custom artwork for this configure dialog
                     * says anything about this input */
					pr = NULL;
					for (i = 0; customizations[i].ipt != IPT_END; i++)
					{
						if (field->type == customizations[i].ipt)
						{
							pr = (RECT *) alloca(sizeof(*pr));
							pr->left = customizations[i].x;
							pr->top = customizations[i].y;
							pr->right = pr->left + customizations[i].width;
							pr->bottom = pr->top + customizations[i].height;
							break;
						}
					}

					/* store this InputPort/RECT combo in our list.  we do not
                     * necessarily want to add it yet because we the INI might
                     * want to reorder the tab order */
					if (customizations[i].ipt == IPT_END)
						i = portslot_count++;
					if (i < ARRAY_LENGTH(portslots))
					{
						portslots[i].field = field;
						portslots[i].pr = pr;
					}
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
			if (win_dialog_add_portselect(dlg, portslots[i].field, portslots[i].pr))
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

static void customize_joystick(running_machine *machine, HWND wnd, int joystick_num)
{
	customize_input(machine, wnd, "Joysticks/Controllers", ARTWORK_CUSTTYPE_JOYSTICK, joystick_num, INPUT_CLASS_CONTROLLER, NULL);
}



//============================================================
//  customize_keyboard
//============================================================

static void customize_keyboard(running_machine *machine, HWND wnd)
{
	customize_input(machine, wnd, "Emulated Keyboard", ARTWORK_CUSTTYPE_KEYBOARD, 0, INPUT_CLASS_KEYBOARD, NULL);
}



//============================================================
//  customize_miscinput
//============================================================

static void customize_miscinput(running_machine *machine, HWND wnd)
{
	customize_input(machine, wnd, "Miscellaneous Input", ARTWORK_CUSTTYPE_MISC, 0, INPUT_CLASS_MISC, NULL);
}



//============================================================
//  customize_categorizedinput
//============================================================

static void customize_categorizedinput(running_machine *machine, HWND wnd, const char *section, int category)
{
	assert(category > 0);
	customize_input(machine, wnd, "Input", ARTWORK_CUSTTYPE_JOYSTICK, category, INPUT_CLASS_CATEGORIZED, section);
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

static void customize_switches(running_machine *machine, HWND wnd, const char* title_string, UINT32 ipt_name)
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

	for (port = machine->portlist.first(); port != NULL; port = port->next)
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

static void customize_dipswitches(running_machine *machine, HWND wnd)
{
	customize_switches(machine, wnd, "Dip Switches", IPT_DIPSWITCH);
}



//============================================================
//  customize_configuration
//============================================================

static void customize_configuration(running_machine *machine, HWND wnd)
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



static void customize_analogcontrols(running_machine *machine, HWND wnd)
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

	for (port = machine->portlist.first(); port != NULL; port = port->next)
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

char *win_dirname(const char *filename)
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
	DWORD fileproc_flags, void (*mameproc)(running_machine *machine, const char *),
	running_machine *machine)
{
	win_open_file_name ofn;
	char *dir;
	int result = 0;
	char *src;
	char *dst;

	if (state_filename[0])
	{
		dir = win_dirname(state_filename);
	}
	else
	{
		snprintf(state_filename, ARRAY_LENGTH(state_filename),
			"%s State.sta", machine->gamedrv->description);
		dir = NULL;

		src = state_filename;
		dst = state_filename;
		do
		{
			if ((*src == '\0') || isalnum(*src) || isspace(*src) || strchr("(),.", *src))
				*(dst++) = *src;
		}
		while(*(src++));
	}

	memset(&ofn, 0, sizeof(ofn));
	ofn.type = dlgtype;
	ofn.owner = wnd;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | fileproc_flags;
	ofn.filter = "State Files (*.sta)|*.sta|All Files (*.*)|*.*";
	ofn.initial_directory = dir;

	snprintf(ofn.filename, ARRAY_LENGTH(ofn.filename), "%s", state_filename);

	result = win_get_file_name_dialog(&ofn);
	if (result)
	{
		snprintf(state_filename, ARRAY_LENGTH(state_filename), "%s", ofn.filename);

		mameproc(machine, state_filename);
	}
	if (dir)
		global_free(dir);
}



static void state_load(HWND wnd, running_machine *machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_OPEN, OFN_FILEMUSTEXIST, mame_schedule_load, machine);
}

static void state_save_as(HWND wnd, running_machine *machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_SAVE, OFN_OVERWRITEPROMPT, mame_schedule_save, machine);
}

static void state_save(running_machine *machine)
{
	mame_schedule_save(machine, state_filename);
}



//============================================================
//  format_combo_changed
//============================================================

struct file_dialog_params
{
	running_device *dev;
	int *create_format;
	option_resolution **create_args;
};

static void format_combo_changed(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *changed_param)
{
	HWND wnd;
	int format_combo_val;
	running_device *dev;
	const option_guide *guide;
	const char *optspec;
	struct file_dialog_params *params;
	int has_option;
	TCHAR t_buf1[128];
	char *buf1;

	params = (struct file_dialog_params *) changed_param;

	// locate the format control
	format_combo_val = notification ? (((OFNOTIFY *) notification)->lpOFN->nFilterIndex - 1) : 0;
	if (format_combo_val < 0)
		format_combo_val = 0;
	*(params->create_format) = format_combo_val;

	// compute our parameters
	dev = params->dev;
	guide = image_device_get_creation_option_guide(dev);
	optspec = image_device_get_indexed_creatable_format(dev, format_combo_val)->optspec;

	// set the default extension
	CommDlg_OpenSave_SetDefExt(GetParent(dlgwnd),
		image_device_get_indexed_creatable_format(dev, format_combo_val)->extensions);

	// enumerate through all of the child windows
	wnd = NULL;
	while((wnd = FindWindowEx(dlgwnd, wnd, NULL, NULL)) != NULL)
	{
		// get label text, removing trailing NULL
		GetWindowText(wnd, t_buf1, ARRAY_LENGTH(t_buf1));
		buf1 = utf8_from_tstring(t_buf1);
		assert(buf1[strlen(buf1)-1] == ':');
		buf1[strlen(buf1)-1] = '\0';

		// find guide entry
		while(guide->option_type && strcmp(buf1, guide->display_name))
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
		global_free(buf1);
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
	running_device *dev;
	char buf[16];

	params = (struct storeval_optres_params *) storeval_param;
	dev = params->fdparams->dev;

	// create the resolution, if necessary
	resolution = *(params->fdparams->create_args);
	if (!resolution)
	{
		const option_guide *optguide = image_device_get_creation_option_guide(dev);
		const image_device_format *format = image_device_get_indexed_creatable_format(dev, *(params->fdparams->create_format));

		resolution = option_resolution_create(optguide, format->optspec);
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

static dialog_box *build_option_dialog(running_device *dev, char *filter, size_t filter_len, int *create_format, option_resolution **create_args)
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
	for (format = image_device_get_creatable_formats(dev); format != NULL; format = format->next)
	{
		pos += add_filter_entry(filter + pos, filter_len - pos, format->description, format->extensions);
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
	for (guide_entry = image_device_get_creation_option_guide(dev); guide_entry->option_type != OPTIONTYPE_END; guide_entry++)
	{
		// make sure that this entry is present on at least one option specification
		found = FALSE;
		for (format = image_device_get_creatable_formats(dev); format != NULL; format = format->next)
		{
			if (option_resolution_contains(format->optspec, guide_entry->parameter))
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

static void build_generic_filter(running_device *device, int is_save, char *filter, size_t filter_len)
{
	char *s;

	const char *file_extension;

	/* copy the string */
	file_extension = device->get_config_string(DEVINFO_STR_IMAGE_FILE_EXTENSIONS);

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

static void change_device(HWND wnd, running_device *device, int is_save)
{
	dialog_box *dialog = NULL;
	char filter[2048];
	char filename[MAX_PATH];
	char buffer[512];
	const char *initial_dir;
	BOOL result;
	int create_format = 0;
	option_resolution *create_args = NULL;
	image_error_t err;
	image_device_info info;

	// sanity check
	assert(device != NULL);

	// get device info
	info = image_device_getinfo(device->machine->config, device);

	// get the file
	if (image_exists(device))
	{
		const char *imgname;
		imgname = image_basename(device);
		snprintf(filename, ARRAY_LENGTH(filename), "%s", imgname);
	}
	else
	{
		filename[0] = '\0';
	}

	// use image directory, if it is there
	initial_dir = image_working_directory(device);

	// add custom dialog elements, if appropriate
	if (is_save
		&& (image_device_get_creation_option_guide(device) != NULL)
		&& (image_device_get_creatable_formats(device) != NULL))
	{
		dialog = build_option_dialog(device, filter, ARRAY_LENGTH(filter), &create_format, &create_args);
		if (!dialog)
			goto done;
	}
	else
	{
		// build a normal filter
		build_generic_filter(device, is_save, filter, ARRAY_LENGTH(filter));
	}

	// display the dialog
	result = win_file_dialog(device->machine, wnd, is_save ? WIN_FILE_DIALOG_SAVE : WIN_FILE_DIALOG_OPEN,
		dialog, filter, initial_dir, filename, ARRAY_LENGTH(filename));
	if (result)
	{
		// mount the image
		if (is_save)
			err = (image_error_t)image_create(device, filename, image_device_get_indexed_creatable_format(device, create_format), create_args);
		else
			err = (image_error_t)image_load(device, filename);

		// error?
		if (err)
		{
			snprintf(buffer, ARRAY_LENGTH(buffer),
				"Error when %s the image: %s",
				is_save ? "creating" : "loading",
				image_error(device));

			win_message_box_utf8(wnd, buffer, APPNAME, MB_OK);
		}
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

static void pause(running_machine *machine)
{
	mame_pause(machine, !winwindow_ui_is_paused(machine));
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
				global_free(t_menutext);
				return NULL;
			}
		}
		while(_tcscmp(t_menutext, buf));

		global_free(t_menutext);

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

#ifdef UNDER_CE
	result = EnableMenuItem(menu_bar, command, (state & MFS_GRAYED ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
	if (result)
		result = CheckMenuItem(menu_bar, command, (state & MFS_CHECKED ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND) != 0xffffffff;
#else
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	result = SetMenuItemInfo(menu_bar, command, FALSE, &mii);
#endif
}



//============================================================
//  append_menu_utf8
//============================================================

static void append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *str)
{
	TCHAR *t_str = str ? tstring_from_utf8(str) : NULL;
	AppendMenu(menu, flags, id, t_str);
	if (t_str)
		global_free(t_str);
}



//============================================================
//  append_menu_uistring
//============================================================

static void append_menu_uistring(HMENU menu, UINT flags, UINT_PTR id, const char *uistring)
{
	append_menu_utf8(menu, flags, id, uistring);
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

static void setup_joystick_menu(running_machine *machine, HMENU menu_bar)
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
	for (port = machine->portlist.first(); port != NULL; port = port->next)
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
		for (port = machine->portlist.first(); port != NULL; port = port->next)
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
						append_menu_utf8(submenu, MF_STRING, command, setting->name);
					}

					// tack on the final items and the menu item
					command = ID_INPUT_0 + serial_number_from_input_item(machine, port, field, NULL);
					AppendMenu(submenu, MF_SEPARATOR, 0, NULL);
					AppendMenu(submenu, MF_STRING, command, TEXT("&Configure..."));
					append_menu_utf8(joystick_menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, field->name);
					child_count++;
				}
			}
		}
	}
	else
	{
		// set up joystick menu
#ifndef UNDER_CE
		joystick_count = input_count_players(machine);
#endif
		if (joystick_count > 0)
		{
			for (i = 0; i < joystick_count; i++)
			{
				snprintf(buf, ARRAY_LENGTH(buf), "Joystick %i", i + 1);
				append_menu_utf8(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, buf);
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

static int frameskip_level_count(void)
{
	static int count = -1;

	if (count < 0)
	{
		int frameskip_min = 0;
		int frameskip_max = 10;
		options_get_range_int(mame_options(), OPTION_FRAMESKIP, &frameskip_min, &frameskip_max);
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
	UINT flags;
	UINT flags_for_exists;
	UINT flags_for_writing;
	running_device *img;
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
		setup_joystick_menu(window->machine, menu_bar);
		joystick_menu_setup = 1;
	}

	frameskip = video_get_frameskip();

	orientation = render_target_get_orientation(window->target);

	speed = video_get_throttle() ? video_get_speed_factor() : 0;

	has_config		= input_has_input_class(window->machine, INPUT_CLASS_CONFIG);
	has_dipswitch	= input_has_input_class(window->machine, INPUT_CLASS_DIPSWITCH);
	has_keyboard	= input_has_input_class(window->machine, INPUT_CLASS_KEYBOARD);
	has_misc		= input_has_input_class(window->machine, INPUT_CLASS_MISC);

	has_analog = 0;
	for (port = window->machine->portlist.first(); port != NULL; port = port->next)
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

	set_command_state(menu_bar, ID_EDIT_PASTE,				inputx_can_post(window->machine)			? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_OPTIONS_PAUSE,			winwindow_ui_is_paused(window->machine)		? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_CONFIGURATION,	has_config									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_DIPSWITCHES,		has_dipswitch								? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_MISCINPUT,		has_misc									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_ANALOGCONTROLS,	has_analog									? MFS_ENABLED : MFS_GRAYED);
#if HAS_TOGGLEFULLSCREEN
	set_command_state(menu_bar, ID_OPTIONS_FULLSCREEN,		!is_windowed()								? MFS_CHECKED : MFS_ENABLED);
#endif // HAS_TOGGLEFULLSCREEN
	set_command_state(menu_bar, ID_OPTIONS_TOGGLEFPS,		ui_get_show_fps()							? MFS_CHECKED : MFS_ENABLED);
#if HAS_PROFILER
	set_command_state(menu_bar, ID_OPTIONS_PROFILER,		ui_get_show_profiler()						? MFS_CHECKED : MFS_ENABLED);
#endif

	set_command_state(menu_bar, ID_KEYBOARD_EMULATED,		(has_keyboard) ?
																(!ui_get_use_natural_keyboard(window->machine)					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_NATURAL,		(has_keyboard && inputx_can_post(window->machine)) ?																(ui_get_use_natural_keyboard(window->machine)					? MFS_CHECKED : MFS_ENABLED)
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
	for (i = 0; i < frameskip_level_count(); i++)
		set_command_state(menu_bar, ID_FRAMESKIP_0 + i,		(frameskip == i)							? MFS_CHECKED : MFS_ENABLED);

	// if we are using categorized input, we need to properly checkmark the categories
	if (use_input_categories)
	{
		for (port = window->machine->portlist.first(); port != NULL; port = port->next)
		{
			for (field = port->fieldlist; field != NULL; field = field->next)
			{
				if (field->type == IPT_CATEGORY)
				{
					input_field_get_user_settings(field, &settings);
					for (setting = field->settinglist; setting != NULL; setting = setting->next)
					{
						command = ID_INPUT_0 + serial_number_from_input_item(window->machine, port, field, setting);
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
	view_index = render_target_get_view(window->target);
	while((view_name = render_target_get_view_name(window->target, i)) != NULL)
	{
		TCHAR *t_view_name = tstring_from_utf8(view_name);
		InsertMenu(video_menu, i, MF_BYPOSITION | (i == view_index ? MF_CHECKED : 0),
			ID_VIDEO_VIEW_0 + i, t_view_name);
		global_free(t_view_name);
		i++;
	}

	// set up device menu; first remove all existing menu items
	device_menu = find_sub_menu(menu_bar, "&Devices\0", FALSE);
	remove_menu_items(device_menu);

	// then set up the actual devices
	for ( img = window->machine->devicelist.first();  img != NULL;  img =  img->next)
	{
		if (is_image_device( img))
		{
			image_device_info info = image_device_getinfo(window->machine->config, img);

			new_item = ID_DEVICE_0 + (image_absolute_index(img) * DEVOPTION_MAX);
			flags_for_exists = MF_STRING;

			if (!image_exists(img))
				flags_for_exists |= MF_GRAYED;

			flags_for_writing = flags_for_exists;
			if (!image_is_writable(img))
				flags_for_writing |= MF_GRAYED;

			sub_menu = CreateMenu();
			append_menu_uistring(sub_menu, MF_STRING,		new_item + DEVOPTION_OPEN,		"Mount...");

			if (info.creatable)
				append_menu_uistring(sub_menu, MF_STRING,	new_item + DEVOPTION_CREATE,	"Create...");

			append_menu_uistring(sub_menu, flags_for_exists,	new_item + DEVOPTION_CLOSE,	"Unmount");

			if ((img->type == CASSETTE) && !strcmp(info.file_extensions, "wav"))
			{
				cassette_state state;
				state = (cassette_state)(image_exists(img) ? (cassette_get_state(img) & CASSETTE_MASK_UISTATE) : CASSETTE_STOPPED);
				append_menu_uistring(sub_menu, MF_SEPARATOR, 0, NULL);
				append_menu_uistring(sub_menu, flags_for_exists	| ((state == CASSETTE_STOPPED)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_STOPPAUSE,	"Pause/Stop");
				append_menu_uistring(sub_menu, flags_for_exists	| ((state == CASSETTE_PLAY)		? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_PLAY,			"Play");
				append_menu_uistring(sub_menu, flags_for_writing	| ((state == CASSETTE_RECORD)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_RECORD,		"Record");
				append_menu_uistring(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_REWIND,		"Rewind");
				append_menu_uistring(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_FASTFORWARD,	"Fast Forward");
			}
			s = image_exists(img) ? image_filename(img) : "[empty slot]";
			flags = MF_POPUP;

			snprintf(buf, ARRAY_LENGTH(buf), "%s: %s", image_typename_id(img), s);
			append_menu_utf8(device_menu, flags, (UINT_PTR) sub_menu, buf);
		}
	}
}



//============================================================
//  set_seped
//============================================================

static void set_speed(int speed)
{
	if (speed != 0)
		video_set_speed_factor(speed);
	video_set_throttle(speed != 0);
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

static void device_command(HWND wnd, running_device *img, int devoption)
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
			image_unload(img);
			break;

		default:
			if (img->type == CASSETTE)
			{
				switch(devoption)
				{
					case DEVOPTION_CASSETTE_STOPPAUSE:
						cassette_change_state(img, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_PLAY:
						cassette_change_state(img, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_RECORD:
						cassette_change_state(img, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_REWIND:
						cassette_seek(img, -1.0, SEEK_CUR);
						break;

					case DEVOPTION_CASSETTE_FASTFORWARD:
						cassette_seek(img, +1.0, SEEK_CUR);
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
		global_free(t_chapter);
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

static void help_about_thissystem(running_machine *machine, HWND wnd)
{
	char buf[256];
	snprintf(buf, ARRAY_LENGTH(buf), "mess.chm::/sysinfo/%s.htm", machine->gamedrv->name);
	help_display(wnd, buf);
}



//============================================================
//  decode_deviceoption
//============================================================

static running_device *decode_deviceoption(running_machine *machine, int command, int *devoption)
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
	render_target_set_orientation(window->target, orientation);
	if (window->target == render_get_ui_target())
	{
		render_container_user_settings settings;
		render_container_get_user_settings(render_container_get_ui(), &settings);
		settings.orientation = orientation;
		render_container_set_user_settings(render_container_get_ui(), &settings);
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
	int handled = 1;
	int dev_command;
	running_device *img;
	UINT16 category;
	const char *section;
	const input_field_config *field;
	const input_setting_config *setting;
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	input_field_user_settings settings;

	// pause while invoking certain commands
	if (pause_for_command(command))
		winwindow_ui_pause_from_window_thread(window->machine, TRUE);

	switch(command)
	{
		case ID_FILE_LOADSTATE_NEWUI:
			state_load(wnd, window->machine);
			break;

		case ID_FILE_SAVESTATE:
			state_save(window->machine);
			break;

		case ID_FILE_SAVESTATE_AS:
			state_save_as(wnd, window->machine);
			break;

		case ID_FILE_SAVESCREENSHOT:
			video_save_active_screen_snapshots(window->machine);
			break;

		case ID_FILE_EXIT_NEWUI:
			mame_schedule_exit(window->machine);
			break;

		case ID_EDIT_PASTE:
			PostMessage(wnd, WM_PASTE, 0, 0);
			break;

		case ID_KEYBOARD_NATURAL:
			ui_set_use_natural_keyboard(window->machine, TRUE);
			break;

		case ID_KEYBOARD_EMULATED:
			ui_set_use_natural_keyboard(window->machine, FALSE);
			break;

		case ID_KEYBOARD_CUSTOMIZE:
			customize_keyboard(window->machine, wnd);
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
			pause(window->machine);
			break;

		case ID_OPTIONS_HARDRESET:
			mame_schedule_hard_reset(window->machine);
			break;

		case ID_OPTIONS_SOFTRESET:
			mame_schedule_soft_reset(window->machine);
			break;

#if HAS_PROFILER
		case ID_OPTIONS_PROFILER:
			ui_set_show_profiler(!ui_get_show_profiler());
			break;
#endif // HAS_PROFILER

		case ID_OPTIONS_DEBUGGER:
			debug_cpu_halt_on_next_instruction(debug_cpu_get_visible_cpu(window->machine), "User-initiated break\n");
			break;

		case ID_OPTIONS_CONFIGURATION:
			customize_configuration(window->machine, wnd);
			break;

		case ID_OPTIONS_DIPSWITCHES:
			customize_dipswitches(window->machine, wnd);
			break;

		case ID_OPTIONS_MISCINPUT:
			customize_miscinput(window->machine, wnd);
			break;

		case ID_OPTIONS_ANALOGCONTROLS:
			customize_analogcontrols(window->machine, wnd);
			break;

#if HAS_TOGGLEFULLSCREEN
		case ID_OPTIONS_FULLSCREEN:
			winwindow_toggle_full_screen();
			break;
#endif

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

#if HAS_TOGGLEMENUBAR
		case ID_OPTIONS_TOGGLEMENUBAR:
			win_toggle_menubar();
			break;
#endif

		case ID_FRAMESKIP_AUTO:
			video_set_frameskip(-1);
			break;

		case ID_HELP_ABOUT_NEWUI:
			help_about_mess(wnd);
			break;

		case ID_HELP_ABOUTSYSTEM:
			help_about_thissystem(window->machine, wnd);
			break;

		case ID_THROTTLE_50:
			set_speed(50);
			break;

		case ID_THROTTLE_100:
			set_speed(100);
			break;

		case ID_THROTTLE_200:
			set_speed(200);
			break;

		case ID_THROTTLE_500:
			set_speed(500);
			break;

		case ID_THROTTLE_1000:
			set_speed(1000);
			break;

		case ID_THROTTLE_UNTHROTTLED:
			set_speed(0);
			break;

		default:
			if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + frameskip_level_count()))
			{
				// change frameskip
				video_set_frameskip(command - ID_FRAMESKIP_0);
			}
			else if ((command >= ID_DEVICE_0) && (command < ID_DEVICE_0 + (IO_COUNT*DEVOPTION_MAX)))
			{
				// change device
				img = decode_deviceoption(window->machine, command, &dev_command);
				device_command(wnd, img, dev_command);
			}
			else if ((command >= ID_JOYSTICK_0) && (command < ID_JOYSTICK_0 + MAX_JOYSTICKS))
			{
				// customize joystick
				customize_joystick(window->machine, wnd, command - ID_JOYSTICK_0);
			}
			else if ((command >= ID_VIDEO_VIEW_0) && (command < ID_VIDEO_VIEW_0 + 1000))
			{
				// render views
				render_target_set_view(window->target, command - ID_VIDEO_VIEW_0);
			}
			else if (input_item_from_serial_number(window->machine, command - ID_INPUT_0, NULL, &field, &setting))
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
					section = NULL;

					for (setting = field->settinglist; setting != NULL; setting = setting->next)
					{
						if (settings.value == setting->value)
						{
							category = setting->category;
							section = setting->name;
						}
					}
					if (category)
						customize_categorizedinput(window->machine, wnd, section, category);
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
		winwindow_ui_pause_from_window_thread(window->machine, FALSE);

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
	global_free(t_text);
}



//============================================================
//  win_setup_menus
//============================================================

int win_setup_menus(running_machine *machine, HMODULE module, HMENU menu_bar)
{
	HMENU frameskip_menu;
	char buf[256];
	int i;

	static const int bitmap_ids[][2] =
	{
		{ IO_CARTSLOT,	IDI_ICON_CART },
		{ IO_HARDDISK,	IDI_ICON_HARD },
		{ IO_CASSETTE,	IDI_ICON_CASS },
		{ IO_FLOPPY,	IDI_ICON_FLOP },
		{ IO_PRINTER,	IDI_ICON_PRIN },
		{ IO_SERIAL,	IDI_ICON_SERL },
		{ IO_SNAPSHOT,	IDI_ICON_SNAP }
	};

	// verify that our magic numbers work
	assert((ID_DEVICE_0 + IO_COUNT * DEVOPTION_MAX) < ID_JOYSTICK_0);

	// initialize critical values
	joystick_menu_setup = 0;

	// get the device icons
	memset(device_icons, 0, sizeof(device_icons));
	for (i = 0; i < ARRAY_LENGTH(bitmap_ids); i++)
		device_icons[bitmap_ids[i][0]] = LoadIcon(module, MAKEINTRESOURCE(bitmap_ids[i][1]));

	// remove the profiler menu item if it doesn't exist
#if HAS_PROFILER
	ui_set_show_profiler(0);
#else
	DeleteMenu(menu_bar, ID_OPTIONS_PROFILER, MF_BYCOMMAND);
#endif

	if ((machine->debug_flags & DEBUG_FLAG_ENABLED) == 0)
		DeleteMenu(menu_bar, ID_OPTIONS_DEBUGGER, MF_BYCOMMAND);

#if !HAS_TOGGLEFULLSCREEN
	DeleteMenu(menu_bar, ID_OPTIONS_FULLSCREEN, MF_BYCOMMAND);
#endif

#if !HAS_TOGGLEMENUBAR
	DeleteMenu(menu_bar, ID_OPTIONS_TOGGLEMENUBAR, MF_BYCOMMAND);
#endif

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0", FALSE);
	if (!frameskip_menu)
		return 1;
	for(i = 0; i < frameskip_level_count(); i++)
	{
		snprintf(buf, ARRAY_LENGTH(buf), "%i", i);
		append_menu_utf8(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf);
	}

	// set the help menu to refer to this machine
	snprintf(buf, ARRAY_LENGTH(buf), "About %s (%s)...", machine->gamedrv->description, machine->gamedrv->name);
	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, buf);
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

#ifdef HAS_WINDOW_MENU
int win_create_menu(running_machine *machine, HMENU *menus)
{
	HMENU menu_bar = NULL;
	HMODULE module;

	if (ui_use_new_ui())
	{
		module = win_resource_module();
		menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
		if (!menu_bar)
			goto error;

		if (win_setup_menus(machine, module, menu_bar))
			goto error;
	}

	*menus = menu_bar;
	return 0;

error:
	if (menu_bar)
		DestroyMenu(menu_bar);
	return 1;
}
#endif /* HAS_WINDOW_MENU */



//============================================================
//  win_mess_window_proc
//============================================================

LRESULT CALLBACK win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
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
				ui_paste(window->machine);
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
