//============================================================
//
//	menu.c - Win32 MESS menus handling
//
//============================================================

// standard windows headers
#include <windows.h>
#include <commdlg.h>
#include <winuser.h>
#include <ctype.h>
#include <tchar.h>

// MAME/MESS headers
#include "mame.h"
#include "menu.h"
#include "ui.h"
#include "messres.h"
#include "inputx.h"
#include "windows/video.h"
#include "windows/input.h"
#include "dialog.h"
#include "opcntrl.h"
#include "uitext.h"
#include "strconv.h"
#include "utils.h"
#include "tapedlg.h"
#include "artworkx.h"
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
//	IMPORTS
//============================================================

// from timer.c
extern void win_timer_enable(int enabled);


//============================================================
//	PARAMETERS
//============================================================

#define ID_FRAMESKIP_0				10000
#define ID_DEVICE_0					11000
#define ID_JOYSTICK_0				12000
#define ID_INPUT_0					13000
#define ID_VIDEO_VIEW_0				14000

#define MAX_JOYSTICKS				(8)

#define USE_TAPEDLG	0

enum
{
	DEVOPTION_OPEN,
	DEVOPTION_CREATE,
	DEVOPTION_CLOSE,
	DEVOPTION_CASSETTE_PLAYRECORD,
	DEVOPTION_CASSETTE_STOPPAUSE,
	DEVOPTION_CASSETTE_PLAY,
	DEVOPTION_CASSETTE_RECORD,
#if USE_TAPEDLG
	DEVOPTION_CASSETTE_DIALOG,
#else
	DEVOPTION_CASSETTE_REWIND,
	DEVOPTION_CASSETTE_FASTFORWARD,
#endif
	DEVOPTION_MAX
};

#ifdef MAME_DEBUG
#define HAS_PROFILER	1
#define HAS_DEBUGGER	1
#else
#define HAS_PROFILER	0
#define HAS_DEBUGGER	0
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
//	LOCAL VARIABLES
//============================================================

static int win_use_natural_keyboard;
static HICON device_icons[IO_COUNT];
static int use_input_categories;
static int joystick_menu_setup;
static char state_filename[MAX_PATH];

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions);


//============================================================
//	customize_input
//============================================================

static void customize_input(HWND wnd, const char *title, int cust_type, int player, int inputclass, const char *section)
{
	dialog_box *dlg;
	input_port_entry *in;
	png_info png;
	struct inputform_customization customizations[128];
	RECT *pr;
	int this_inputclass, this_player, portslot_count, i;

	struct
	{
		input_port_entry *in;
		const RECT *pr;
	} portslots[256];

	/* check to see if there is any custom artwork for this configure dialog,
	 * and if so, retrieve the info */
	portslot_count = artwork_get_inputscreen_customizations(&png, cust_type, section,
		customizations, sizeof(customizations) / sizeof(customizations[0]));

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

	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		this_inputclass = input_classify_port(in);
		if (input_port_name(in) && (this_inputclass == inputclass))
		{
			/* most of the time, the player parameter is the player number
			 * but in the case of INPUT_CLASS_CATEGORIZED, it is the
			 * category */
			if (inputclass == INPUT_CLASS_CATEGORIZED)
				this_player = in->category;
			else
                this_player = input_player_number(in);

			if (this_player == player)
			{
				/* check to see if the custom artwork for this configure dialog
				 * says anything about this input */
				pr = NULL;
				for (i = 0; customizations[i].ipt != IPT_END; i++)
				{
					if (in->type == customizations[i].ipt)
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
				if (i < (sizeof(portslots) / sizeof(portslots[0])))
				{
					portslots[i].in = in;
					portslots[i].pr = pr;
				}
			}
		}
		in++;
	}

	/* finally add the portselects to the dialog */
	if (portslot_count > (sizeof(portslots) / sizeof(portslots[0])))
		portslot_count = sizeof(portslots) / sizeof(portslots[0]);
	for (i = 0; i < portslot_count; i++)
	{
		if (portslots[i].in)
		{
			if (win_dialog_add_portselect(dlg, portslots[i].in, portslots[i].pr))
				goto done;
		}
	}

	/* ...and now add OK/Cancel */
	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	/* ...and finally display the dialog */
	win_dialog_runmodal(wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//	customize_joystick
//============================================================

static void customize_joystick(HWND wnd, int joystick_num)
{
	customize_input(wnd, "Joysticks/Controllers", ARTWORK_CUSTTYPE_JOYSTICK, joystick_num, INPUT_CLASS_CONTROLLER, NULL);
}



//============================================================
//	customize_keyboard
//============================================================

static void customize_keyboard(HWND wnd)
{
	customize_input(wnd, "Emulated Keyboard", ARTWORK_CUSTTYPE_KEYBOARD, 0, INPUT_CLASS_KEYBOARD, NULL);
}



//============================================================
//	customize_miscinput
//============================================================

static void customize_miscinput(HWND wnd)
{
	customize_input(wnd, "Miscellaneous Input", ARTWORK_CUSTTYPE_MISC, 0, INPUT_CLASS_MISC, NULL);
}



//============================================================
//	customize_categorizedinput
//============================================================

static void customize_categorizedinput(HWND wnd, const char *section, int category)
{
	assert(category > 0);
	customize_input(wnd, "Input", ARTWORK_CUSTTYPE_JOYSTICK, category, INPUT_CLASS_CATEGORIZED, section);
}



//============================================================
//	storeval_inputport
//============================================================

static void storeval_inputport(void *param, int val)
{
	input_port_entry *in = (input_port_entry *) param;
	in->default_value = (UINT16) val;
}



//============================================================
//	customize_switches
//============================================================

static void customize_switches(HWND wnd, int title_string_num, UINT32 ipt_name, UINT32 ipt_setting)
{
	dialog_box *dlg;
	input_port_entry *in;
	const char *switch_name = NULL;
	UINT32 type;
	
	dlg = win_dialog_init(ui_getstring(title_string_num), NULL);
	if (!dlg)
		goto done;

	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		type = in->type;

		if (type == ipt_name)
		{
			if (input_port_active(in))
			{
				switch_name = input_port_name(in);
				if (win_dialog_add_combobox(dlg, switch_name, in->default_value, storeval_inputport, in))
					goto done;
			}
			else
			{
				switch_name = NULL;
			}
		}
		else if (type == ipt_setting)
		{
			if (switch_name)
			{
				if (win_dialog_add_combobox_item(dlg, input_port_name(in), in->default_value))
					goto done;
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//	customize_dipswitches
//============================================================

static void customize_dipswitches(HWND wnd)
{
	customize_switches(wnd, UI_dipswitches, IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
}



//============================================================
//	customize_configuration
//============================================================

static void customize_configuration(HWND wnd)
{
	customize_switches(wnd, UI_configuration, IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
}



//============================================================
//	customize_analogcontrols
//============================================================

static void store_delta(void *param, int val)
{
	((input_port_entry *) param)->analog.delta = val;
}



static void store_centerdelta(void *param, int val)
{
	((input_port_entry *) param)->analog.centerdelta = val;
}



static void store_reverse(void *param, int val)
{
	((input_port_entry *) param)->analog.reverse = val;
}



static void store_sensitivity(void *param, int val)
{
	((input_port_entry *) param)->analog.sensitivity = val;
}



static void customize_analogcontrols(HWND wnd)
{
	dialog_box *dlg;
	input_port_entry *in;
	const char *name;
	char buf[255];
	static const struct dialog_layout layout = { 120, 52 };
	
	dlg = win_dialog_init(ui_getstring(UI_analogcontrols), &layout);
	if (!dlg)
		goto done;

	in = Machine->input_ports;

	while (in->type != IPT_END)
	{
		if (port_type_is_analog(in->type))
		{
			name = input_port_name(in);

			_snprintf(buf, sizeof(buf) / sizeof(buf[0]),
				"%s %s", name, ui_getstring(UI_keyjoyspeed));
			if (win_dialog_add_adjuster(dlg, buf, in->analog.delta, 1, 255, FALSE, store_delta, in))
				goto done;

			_snprintf(buf, sizeof(buf) / sizeof(buf[0]),
				"%s %s", name, ui_getstring(UI_centerspeed));
			if (win_dialog_add_adjuster(dlg, buf, in->analog.centerdelta, 1, 255, FALSE, store_centerdelta, in))
				goto done;

			_snprintf(buf, sizeof(buf) / sizeof(buf[0]),
				"%s %s", name, ui_getstring(UI_reverse));
			if (win_dialog_add_combobox(dlg, buf, in->analog.reverse ? 1 : 0, store_reverse, in))
				goto done;
			if (win_dialog_add_combobox_item(dlg, ui_getstring(UI_off), 0))
				goto done;
			if (win_dialog_add_combobox_item(dlg, ui_getstring(UI_on), 1))
				goto done;

			_snprintf(buf, sizeof(buf) / sizeof(buf[0]),
				"%s %s", name, ui_getstring(UI_sensitivity));
			if (win_dialog_add_adjuster(dlg, buf, in->analog.sensitivity, 1, 255, TRUE, store_sensitivity, in))
				goto done;
		}
		in++;
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//	state_dialog
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
		dir = osd_dirname(state_filename);
	}
	else
	{
		snprintf(state_filename, sizeof(state_filename) / sizeof(state_filename[0]),
			"%s State.sta", Machine->gamedrv->description);
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
		free(dir);
}



void state_load(HWND wnd, running_machine *machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_OPEN, OFN_FILEMUSTEXIST, mame_schedule_load, machine);
}

void state_save_as(HWND wnd, running_machine *machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_SAVE, OFN_OVERWRITEPROMPT, mame_schedule_save, machine);
}

void state_save(running_machine *machine)
{
	mame_schedule_save(machine, state_filename);
}



//============================================================
//	format_combo_changed
//============================================================

struct file_dialog_params
{
	const struct IODevice *dev;
	int *create_format;
	option_resolution **create_args;
};

static void format_combo_changed(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *changed_param)
{
	HWND wnd;
	int format_combo_val;
	const struct IODevice *dev;
	const struct OptionGuide *guide;
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
	guide = dev->createimage_optguide;	
	optspec = dev->createimage_options[format_combo_val].optspec;

	// set the default extension
	CommDlg_OpenSave_SetDefExt(GetParent(dlgwnd),
		dev->createimage_options[format_combo_val].extensions);

	// enumerate through all of the child windows
	wnd = NULL;
	while((wnd = FindWindowEx(dlgwnd, wnd, NULL, NULL)) != NULL)
	{
		// get label text, removing trailing NULL
		GetWindowText(wnd, t_buf1, sizeof(t_buf1) / sizeof(t_buf1[0]));
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
		free(buf1);
	}
}



//============================================================
//	storeval_option_resolution
//============================================================

struct storeval_optres_params
{
	struct file_dialog_params *fdparams;
	const struct OptionGuide *guide_entry;
};

static void storeval_option_resolution(void *storeval_param, int val)
{
	option_resolution *resolution;
	struct storeval_optres_params *params;
	const struct IODevice *dev;
	char buf[16];
	
	params = (struct storeval_optres_params *) storeval_param;
	dev = params->fdparams->dev;

	// create the resolution, if necessary
	resolution = *(params->fdparams->create_args);
	if (!resolution)
	{
		resolution = option_resolution_create(dev->createimage_optguide, dev->createimage_options[*(params->fdparams->create_format)].optspec);
		if (!resolution)
			return;
		*(params->fdparams->create_args) = resolution;
	}

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%d", val);
	option_resolution_add_param(resolution, params->guide_entry->identifier, buf);
}



//============================================================
//	build_option_dialog
//============================================================

static dialog_box *build_option_dialog(const struct IODevice *dev, char *filter, size_t filter_len, int *create_format, option_resolution **create_args)
{
	dialog_box *dialog;
	const struct OptionGuide *guide_entry;
	int found, i, pos;
	char buf[256];
	struct file_dialog_params *params;
	struct storeval_optres_params *storeval_params;
	const struct dialog_layout filedialog_layout = { 44, 220 };

	// make the filter
	pos = 0;
	for (i = 0; dev->createimage_options[i].optspec; i++)
	{
		pos += add_filter_entry(filter + pos, filter_len - pos,
			dev->createimage_options[i].description,
			dev->createimage_options[i].extensions);
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
	for (guide_entry = dev->createimage_optguide; guide_entry->option_type != OPTIONTYPE_END; guide_entry++)
	{
		// make sure that this entry is present on at least one option specification
		found = FALSE;
		for (i = 0; dev->createimage_options[i].optspec; i++)
		{
			if (option_resolution_contains(dev->createimage_options[i].optspec, guide_entry->parameter))
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
			switch(guide_entry->option_type) {
			case OPTIONTYPE_INT:
				snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s:", guide_entry->display_name);
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
//	copy_extension_list
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
//	add_filter_entry
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
//	build_generic_filter
//============================================================

static void build_generic_filter(const struct IODevice *dev, int is_save, char *filter, size_t filter_len)
{
	char *s;
	const char *file_extensions;

	s = filter;

	// common image types
	file_extensions = device_get_info_string(&dev->devclass, DEVINFO_STR_FILE_EXTENSIONS);
	s += add_filter_entry(filter, filter_len, "Common image types", file_extensions);

	// all files
	s += sprintf(s, "All files (*.*)|*.*|");

	// compressed
	if (!is_save)
		s += sprintf(s, "Compressed Images (*.zip)|*.zip|");

	*(s++) = '\0';
}



//============================================================
//	change_device
//============================================================

static void change_device(HWND wnd, mess_image *img, int is_save)
{
	dialog_box *dialog = NULL;
	char filter[2048];
	char filename[MAX_PATH];
	char buffer[512];
	const struct IODevice *dev = image_device(img);
	const char *initial_dir;
	BOOL result;
	int create_format = 0;
	option_resolution *create_args = NULL;
	image_error_t err;

	assert(dev);

	// get the file
	if (image_exists(img))
	{
		const char *imgname;
		imgname = image_basename(img);
		strncpyz(filename, imgname, sizeof(filename) / sizeof(filename[0]));
	}
	else
	{
		filename[0] = '\0';
	}

	// use image directory, if it is there
	get_devicedirectory(dev->type);
	initial_dir = image_working_directory(img);

	// add custom dialog elements, if appropriate
	if (is_save && dev->createimage_optguide && dev->createimage_options[0].optspec)
	{
		dialog = build_option_dialog(dev, filter, sizeof(filter) / sizeof(filter[0]), &create_format, &create_args);
		if (!dialog)
			goto done;
	}
	else
	{
		// build a normal filter
		build_generic_filter(dev, is_save, filter, sizeof(filter) / sizeof(filter[0]));
	}

	// display the dialog
	result = win_file_dialog(wnd, is_save ? WIN_FILE_DIALOG_SAVE : WIN_FILE_DIALOG_OPEN,
		dialog, filter, initial_dir, filename, sizeof(filename) / sizeof(filename[0]));
	if (result)
	{
		// mount the image
		if (is_save)
			err = image_create(img, filename, create_format, create_args);
		else
			err = image_load(img, filename);

		// error?
		if (err)
		{
			snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]),
				"Error when %s the image: %s",
				is_save ? "creating" : "loading",
				image_error(img));

			win_message_box_utf8(wnd, buffer, APPNAME, MB_OK);
		}
	}

done:
	if (dialog)
		win_dialog_exit(dialog);
	if (create_args)
		option_resolution_close(create_args);
}



//============================================================
//	paste
//============================================================

static void paste(void)
{
	HANDLE h;
	LPSTR text;
	size_t mb_size;
	size_t w_size;
	LPWSTR wtext;

	if (!OpenClipboard(NULL))
		return;
	
	h = GetClipboardData(CF_TEXT);
	if (h)
	{
		text = GlobalLock(h);
		if (text)
		{
			mb_size = strlen(text);
			w_size = MultiByteToWideChar(CP_ACP, 0, text, mb_size, NULL, 0);
			wtext = (LPWSTR) alloca(w_size * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, text, mb_size, wtext, w_size);
			inputx_postn_utf16(wtext, w_size);
			GlobalUnlock(h);
		}
	}

	CloseClipboard();
}



//============================================================
//	pause
//============================================================

static void pause(running_machine *machine)
{
	mame_pause(machine, !mame_is_paused(machine));
}



//============================================================
//	get_menu_item_string
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
//	find_sub_menu
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
				free(t_menutext);
				return NULL;
			}
		}
		while(_tcscmp(t_menutext, buf));

		free(t_menutext);

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
//	set_command_state
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
//	append_menu_utf8
//============================================================

static void append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *str)
{
	TCHAR *t_str = str ? tstring_from_utf8(str) : NULL;
	AppendMenu(menu, flags, id, t_str);
	if (t_str)
		free(t_str);
}



//============================================================
//	append_menu_uistring
//============================================================

static void append_menu_uistring(HMENU menu, UINT flags, UINT_PTR id, int uistring)
{
	append_menu_utf8(menu, flags, id, (uistring >= 0) ? ui_getstring(uistring) : NULL);
}



//============================================================
//	remove_menu_items
//============================================================

static void remove_menu_items(HMENU menu)
{
	while(RemoveMenu(menu, 0, MF_BYPOSITION))
		;
}



//============================================================
//	setup_joystick_menu
//============================================================

static void setup_joystick_menu(HMENU menu_bar)
{
	int joystick_count = 0;
	HMENU joystick_menu;
	int i, j;
	HMENU submenu = NULL;
	const input_port_entry *in;
	const input_port_entry *in_setting;
	char buf[256];
	int child_count = 0;

	in = Machine->input_ports;
	use_input_categories = 0;
	while(in->type != IPT_END)
	{
		if (in->type == IPT_CATEGORY_NAME)
		{
			use_input_categories = 1;
			break;
		}
		in++;
	}

	joystick_menu = find_sub_menu(menu_bar, "&Options\0&Joysticks\0", TRUE);
	if (!joystick_menu)
		return;

	if (use_input_categories)
	{
		// using input categories
		for (i = 0; Machine->input_ports[i].type != IPT_END; i++)
		{
			in = &Machine->input_ports[i];
			if ((in->type) == IPT_CATEGORY_NAME)
			{
				submenu = CreateMenu();
				if (!submenu)
					return;

				// append all of the category settings
				for (j = i + 1; (Machine->input_ports[j].type) == IPT_CATEGORY_SETTING; j++)
				{
					in_setting = &Machine->input_ports[j];
					append_menu_utf8(submenu, MF_STRING, ID_INPUT_0 + j, in_setting->name);
				}

				// tack on the final items and the menu item
				AppendMenu(submenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(submenu, MF_STRING, ID_INPUT_0 + i, TEXT("&Configure..."));
				append_menu_utf8(joystick_menu, MF_STRING | MF_POPUP, (UINT_PTR) submenu, in->name);
				child_count++;
			}
		}
	}
	else
	{
		// set up joystick menu
#ifndef UNDER_CE
		joystick_count = input_count_players();
#endif
		if (joystick_count > 0)
		{
			for(i = 0; i < joystick_count; i++)
			{
				snprintf(buf, sizeof(buf) / sizeof(buf[0]), "Joystick %i", i + 1);
				append_menu_utf8(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, buf);
				child_count++;
			}
		}
	}

	// last but not least, enable the joystick menu (or not)
	set_command_state(menu_bar, ID_OPTIONS_JOYSTICKS, child_count ? MFS_ENABLED : MFS_GRAYED);
}



//============================================================
//	is_windowed
//============================================================

static int is_windowed(void)
{
	return video_config.windowed;
}



//============================================================
//	frameskip_level_count
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
//	prepare_menus
//============================================================

static void prepare_menus(HWND wnd)
{
	int i;
	const struct IODevice *dev;
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
	mess_image *img;
	int has_config, has_dipswitch, has_keyboard, has_analog, has_misc;
	const input_port_entry *in;
	UINT16 in_cat_value = 0;
	int frameskip;
	int orientation;
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	const char *view_name;
	int view_index;

	menu_bar = GetMenu(wnd);
	if (!menu_bar)
		return;

	if (!joystick_menu_setup)
	{
		setup_joystick_menu(menu_bar);
		joystick_menu_setup = 1;
	}

	frameskip = video_get_frameskip();

	orientation = render_target_get_orientation(window->target);

	has_config		= input_has_input_class(INPUT_CLASS_CONFIG);
	has_dipswitch	= input_has_input_class(INPUT_CLASS_DIPSWITCH);
	has_keyboard	= input_has_input_class(INPUT_CLASS_KEYBOARD);
	has_misc		= input_has_input_class(INPUT_CLASS_MISC);

	has_analog = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (port_type_is_analog(in->type))
		{
			has_analog = 1;
			break;
		}
	}

	set_command_state(menu_bar, ID_FILE_SAVESTATE,			state_filename[0] != '\0'					? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_EDIT_PASTE,				inputx_can_post()							? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_OPTIONS_PAUSE,			winwindow_ui_is_paused()					? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_THROTTLE,		video_get_throttle()								? MFS_CHECKED : MFS_ENABLED);
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
																(!win_use_natural_keyboard					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_NATURAL,		(has_keyboard && inputx_can_post()) ?
																(win_use_natural_keyboard					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_CUSTOMIZE,		has_keyboard								? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_VIDEO_ROTATE_0,			(orientation == ROT0)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_90,			(orientation == ROT90)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_180,		(orientation == ROT180)						? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_270,		(orientation == ROT270)						? MFS_CHECKED : MFS_ENABLED);

	set_command_state(menu_bar, ID_FRAMESKIP_AUTO,			(frameskip < 0)								? MFS_CHECKED : MFS_ENABLED);
	for (i = 0; i < frameskip_level_count(); i++)
		set_command_state(menu_bar, ID_FRAMESKIP_0 + i,		(frameskip == i)							? MFS_CHECKED : MFS_ENABLED);

	// if we are using categorized input, we need to properly checkmark the categories
	if (use_input_categories)
	{
		for (i = 0; Machine->input_ports[i].type != IPT_END; i++)
		{
			in = &Machine->input_ports[i];
			switch(in->type) {
			case IPT_CATEGORY_NAME:
				in_cat_value = in->default_value;
				break;

			case IPT_CATEGORY_SETTING:
				set_command_state(menu_bar, ID_INPUT_0 + i, (in->default_value == in_cat_value) ? MFS_CHECKED : MFS_ENABLED);
				break;
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
		free(t_view_name);
		i++;
	}

	// set up device menu; first remove all existing menu items
	device_menu = find_sub_menu(menu_bar, "&Devices\0", FALSE);
	remove_menu_items(device_menu);

	// then set up the actual devices
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		for (i = 0; i < dev->count; i++)
		{
			img = image_from_device_and_index(dev, i);

			if (!dev->not_working)
			{	
				new_item = ID_DEVICE_0 + (image_absolute_index(img) * DEVOPTION_MAX);
				flags_for_exists = MF_STRING;
	
				if (!image_exists(img))
					flags_for_exists |= MF_GRAYED;
	
				flags_for_writing = flags_for_exists;
				if (!image_is_writable(img))
					flags_for_writing |= MF_GRAYED;
	
				sub_menu = CreateMenu();
				append_menu_uistring(sub_menu, MF_STRING,		new_item + DEVOPTION_OPEN,		UI_mount);
	
				if (dev->creatable)
					append_menu_uistring(sub_menu, MF_STRING,	new_item + DEVOPTION_CREATE,	UI_create);
	
				append_menu_uistring(sub_menu, flags_for_exists,	new_item + DEVOPTION_CLOSE,	UI_unmount);

#if HAS_WAVE
				if ((dev->type == IO_CASSETTE) && !strcmp(dev->file_extensions, "wav"))
				{
					cassette_state state;
					state = image_exists(img) ? (cassette_get_state(img) & CASSETTE_MASK_UISTATE) : CASSETTE_STOPPED;
					append_menu_uistring(sub_menu, MF_SEPARATOR, 0, -1);
					append_menu_uistring(sub_menu, flags_for_exists	| ((state == CASSETTE_STOPPED)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_STOPPAUSE,	UI_pauseorstop);
					append_menu_uistring(sub_menu, flags_for_exists	| ((state == CASSETTE_PLAY)		? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_PLAY,			UI_play);
					append_menu_uistring(sub_menu, flags_for_writing	| ((state == CASSETTE_RECORD)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_RECORD,		UI_record);
#if USE_TAPEDLG
					append_menu_uistring(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_DIALOG,		UI_tapecontrol);
#else
					append_menu_uistring(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_REWIND,		UI_rewind);
					append_menu_uistring(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_FASTFORWARD,	UI_fastforward);
#endif
				}
#endif /* HAS_WAVE */
				s = image_exists(img) ? image_filename(img) : ui_getstring(UI_emptyslot);
				flags = MF_POPUP;
			}
			else
			{
				sub_menu = NULL;
				s = "Not working";
				flags = MF_DISABLED | MF_GRAYED;
			}

			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s: %s", image_typename_id(img), s);
			append_menu_utf8(device_menu, flags, (UINT_PTR) sub_menu, buf);
		}
	}
}



//============================================================
//	win_toggle_menubar
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
//	device_command
//============================================================

static void device_command(HWND wnd, mess_image *img, int devoption)
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
			switch(image_devtype(img))
			{
#if HAS_WAVE
				case IO_CASSETTE:
					switch(devoption) {
					case DEVOPTION_CASSETTE_STOPPAUSE:
						cassette_change_state(img, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_PLAY:
						cassette_change_state(img, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_RECORD:
						cassette_change_state(img, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
						break;

#if USE_TAPEDLG
					case DEVOPTION_CASSETTE_DIALOG:
						tapedialog_show(wnd, id);
						break;
#else
					case DEVOPTION_CASSETTE_REWIND:
						cassette_seek(img, +1.0, SEEK_CUR);
						break;

					case DEVOPTION_CASSETTE_FASTFORWARD:
						cassette_seek(img, +1.0, SEEK_CUR);
						break;
#endif
				}
				break;
#endif /* HAS_WAVE */

			default:
				break;
		}
	}
}



//============================================================
//	help_display
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
		free(t_chapter);
	}
}



//============================================================
//	help_about_mess
//============================================================

static void help_about_mess(HWND wnd)
{
	help_display(wnd, "mess.chm::/windows/main.htm");
}



//============================================================
//	help_about_thissystem
//============================================================

static void help_about_thissystem(HWND wnd)
{
	char buf[256];
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "mess.chm::/sysinfo/%s.htm", Machine->gamedrv->name);
	help_display(wnd, buf);
}



//============================================================
//	decode_deviceoption
//============================================================

static mess_image *decode_deviceoption(int command, int *devoption)
{
	int absolute_index;
	
	command -= ID_DEVICE_0;
	absolute_index = command / DEVOPTION_MAX;

	if (devoption)
		*devoption = command % DEVOPTION_MAX;

	return image_from_absolute_index(absolute_index);
}



//============================================================
//	set_window_orientation
//============================================================

static void set_window_orientation(win_window_info *window, int orientation)
{
	render_target_set_orientation(window->target, orientation);
	if (window->target == render_get_ui_target())
		render_container_set_orientation(render_container_get_ui(), orientation);
	winwindow_video_window_update(window);
}



//============================================================
//	invoke_command
//============================================================

static int invoke_command(HWND wnd, UINT command)
{
	int handled = 1;
	int dev_command, i;
	mess_image *img;
	int port_count;
	UINT16 setting, category;
	input_port_entry *in;
	const char *section;
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;

	switch(command)
	{
		case ID_FILE_LOADSTATE:
			state_load(wnd, Machine);
			break;

		case ID_FILE_SAVESTATE:
			state_save(Machine);
			break;

		case ID_FILE_SAVESTATE_AS:
			state_save_as(wnd, Machine);
			break;

		case ID_FILE_SAVESCREENSHOT:
			video_save_active_screen_snapshots(Machine);
			break;

		case ID_FILE_EXIT:
			mame_schedule_exit(Machine);
			break;

		case ID_EDIT_PASTE:
			paste();
			break;

		case ID_KEYBOARD_NATURAL:
			win_use_natural_keyboard = 1;
			break;

		case ID_KEYBOARD_EMULATED:
			win_use_natural_keyboard = 0;
			break;

		case ID_KEYBOARD_CUSTOMIZE:
			customize_keyboard(wnd);
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
			pause(Machine);
			break;

		case ID_OPTIONS_HARDRESET:
			mame_schedule_hard_reset(Machine);
			break;

		case ID_OPTIONS_SOFTRESET:
			mame_schedule_soft_reset(Machine);
			break;

		case ID_OPTIONS_THROTTLE:
			video_set_throttle(!video_get_throttle());
			break;

#if HAS_PROFILER
		case ID_OPTIONS_PROFILER:
			ui_set_show_profiler(!ui_get_show_profiler());
			break;
#endif // HAS_PROFILER

#if HAS_DEBUGGER
		case ID_OPTIONS_DEBUGGER:
			debug_halt_on_next_instruction();
			break;
#endif // HAS_DEBUGGER

		case ID_OPTIONS_CONFIGURATION:
			customize_configuration(wnd);
			break;

		case ID_OPTIONS_DIPSWITCHES:
			customize_dipswitches(wnd);
			break;

		case ID_OPTIONS_MISCINPUT:
			customize_miscinput(wnd);
			break;

		case ID_OPTIONS_ANALOGCONTROLS:
			customize_analogcontrols(wnd);
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
//				extern int win_use_mouse;
//				win_use_mouse = !win_use_mouse;
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

		case ID_HELP_ABOUT:
			help_about_mess(wnd);
			break;

		case ID_HELP_ABOUTSYSTEM:
			help_about_thissystem(wnd);
			break;

		default:
			// quickly come up with a port count, so we can upper bound commands
			// near ID_INPUT_0
			port_count = 0;
			while(Machine->input_ports[port_count].type != IPT_END)
				port_count++;

			if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + frameskip_level_count()))
			{
				// change frameskip
				video_set_frameskip(command - ID_FRAMESKIP_0);
			}
			else if ((command >= ID_DEVICE_0) && (command < ID_DEVICE_0 + (MAX_DEV_INSTANCES*IO_COUNT*DEVOPTION_MAX)))
			{
				// change device
				img = decode_deviceoption(command, &dev_command);
				device_command(wnd, img, dev_command);
			}
			else if ((command >= ID_JOYSTICK_0) && (command < ID_JOYSTICK_0 + MAX_JOYSTICKS))
			{
				// customize joystick
				customize_joystick(wnd, command - ID_JOYSTICK_0);
			}
			else if ((command >= ID_VIDEO_VIEW_0) && (command < ID_VIDEO_VIEW_0 + 1000))
			{
				// render views
				render_target_set_view(window->target, command - ID_VIDEO_VIEW_0);
			}
			else if ((command >= ID_INPUT_0) && (command < ID_INPUT_0 + port_count))
			{
				// customize categorized input
				in = &Machine->input_ports[command - ID_INPUT_0];
				switch(in->type) {
				case IPT_CATEGORY_NAME:
					// customize the input type
					category = 0;
					section = NULL;
					for (i = 1; (in[i].type) == IPT_CATEGORY_SETTING; i++)
					{
						if (in[i].default_value == in[0].default_value)
						{
							category = in[i].category;
							section = in[i].name;
						}
					}
					customize_categorizedinput(wnd, section, category);
					break;

				case IPT_CATEGORY_SETTING:
					// change the input type for this category
					setting = in->default_value;
					while((in->type) != IPT_CATEGORY_NAME)
						in--;
					in->default_value = setting;
					break;

				default:
					// should never happen
					handled = 0;
					break;
				}
			}
			else
			{
				// bogus command
				handled = 0;
			}
			break;
	}
	return handled;
}



//============================================================
//	set_menu_text
//============================================================

void set_menu_text(HMENU menu_bar, int command, const char *text)
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
	free(t_text);
}



//============================================================
//	win_setup_menus
//============================================================

int win_setup_menus(HMODULE module, HMENU menu_bar)
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
	assert((ID_DEVICE_0 + IO_COUNT * MAX_DEV_INSTANCES * DEVOPTION_MAX) < ID_JOYSTICK_0);

	// initialize critical values
	joystick_menu_setup = 0;

	// get the device icons
	memset(device_icons, 0, sizeof(device_icons));
	for (i = 0; i < sizeof(bitmap_ids) / sizeof(bitmap_ids[0]); i++)
		device_icons[bitmap_ids[i][0]] = LoadIcon(module, MAKEINTRESOURCE(bitmap_ids[i][1]));

	// remove the profiler menu item if it doesn't exist
#if HAS_PROFILER
	ui_set_show_profiler(0);
#else
	DeleteMenu(menu_bar, ID_OPTIONS_PROFILER, MF_BYCOMMAND);
#endif

	if (!HAS_DEBUGGER || !Machine->debug_mode)
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
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%i", i);
		append_menu_utf8(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf);
	}

	// set the help menu to refer to this machine
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "About %s (%s)...", Machine->gamedrv->description, Machine->gamedrv->name);
	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, buf);
	return 0;
}



//============================================================
//	win_resource_module
//============================================================

HMODULE win_resource_module(void)
{
#ifdef _MSC_VER
	// doing this because of lame build problems with EMULATORDLL, and how
	// vconv is invoked
#ifdef PTR64
	return (HMODULE) 0x180000000;
#else
	return (HMODULE) 0x10000000;
#endif
#else // !_MSC_VER
	static HMODULE module;
	if (!module)
		module = LoadLibrary(TEXT(EMULATORDLL));
	return module;
#endif // _MSC_VER
}



//============================================================
//	win_create_menu
//============================================================

#ifndef UNDER_CE
int win_create_menu(HMENU *menus)
{
	HMENU menu_bar = NULL;
	HMODULE module;

	// determine whether we are using the natural keyboard or not
	win_use_natural_keyboard = options_get_bool(mame_options(), "natural");

	if (mess_use_new_ui())
	{
		module = win_resource_module();
		menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
		if (!menu_bar)
			goto error;

		if (win_setup_menus(module, menu_bar))
			goto error;
	}

	*menus = menu_bar;
	return 0;

error:
	if (menu_bar)
		DestroyMenu(menu_bar);
	return 1;
}
#endif /* UNDER_CE */



//============================================================
//	win_mess_window_proc
//============================================================

LRESULT CALLBACK win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	int i;
	MSG msg;

	static const WPARAM keytrans[][2] =
	{
		{ VK_ESCAPE,	UCHAR_MAMEKEY(ESC) },
		{ VK_F1,		UCHAR_MAMEKEY(F1) },
		{ VK_F2,		UCHAR_MAMEKEY(F2) },
		{ VK_F3,		UCHAR_MAMEKEY(F3) },
		{ VK_F4,		UCHAR_MAMEKEY(F4) },
		{ VK_F5,		UCHAR_MAMEKEY(F5) },
		{ VK_F6,		UCHAR_MAMEKEY(F6) },
		{ VK_F7,		UCHAR_MAMEKEY(F7) },
		{ VK_F8,		UCHAR_MAMEKEY(F8) },
		{ VK_F9,		UCHAR_MAMEKEY(F9) },
		{ VK_F10,		UCHAR_MAMEKEY(F10) },
		{ VK_F11,		UCHAR_MAMEKEY(F11) },
		{ VK_F12,		UCHAR_MAMEKEY(F12) },
		{ VK_NUMLOCK,	UCHAR_MAMEKEY(F12) },
		{ VK_SCROLL,	UCHAR_MAMEKEY(F12) },
		{ VK_NUMPAD0,	UCHAR_MAMEKEY(0_PAD) },
		{ VK_NUMPAD1,	UCHAR_MAMEKEY(1_PAD) },
		{ VK_NUMPAD2,	UCHAR_MAMEKEY(2_PAD) },
		{ VK_NUMPAD3,	UCHAR_MAMEKEY(3_PAD) },
		{ VK_NUMPAD4,	UCHAR_MAMEKEY(4_PAD) },
		{ VK_NUMPAD5,	UCHAR_MAMEKEY(5_PAD) },
		{ VK_NUMPAD6,	UCHAR_MAMEKEY(6_PAD) },
		{ VK_NUMPAD7,	UCHAR_MAMEKEY(7_PAD) },
		{ VK_NUMPAD8,	UCHAR_MAMEKEY(8_PAD) },
		{ VK_NUMPAD9,	UCHAR_MAMEKEY(9_PAD) },
		{ VK_DECIMAL,	UCHAR_MAMEKEY(DEL_PAD) },
		{ VK_ADD,		UCHAR_MAMEKEY(PLUS_PAD) },
		{ VK_SUBTRACT,	UCHAR_MAMEKEY(MINUS_PAD) },
		{ VK_INSERT,	UCHAR_MAMEKEY(INSERT) },
		{ VK_DELETE,	UCHAR_MAMEKEY(DEL) },
		{ VK_HOME,		UCHAR_MAMEKEY(HOME) },
		{ VK_END,		UCHAR_MAMEKEY(END) },
		{ VK_PRIOR,		UCHAR_MAMEKEY(PGUP) },
		{ VK_NEXT,		UCHAR_MAMEKEY(PGDN) },
		{ VK_UP,		UCHAR_MAMEKEY(UP) },
		{ VK_DOWN,		UCHAR_MAMEKEY(DOWN) },
		{ VK_LEFT,		UCHAR_MAMEKEY(LEFT) },
		{ VK_RIGHT,		UCHAR_MAMEKEY(RIGHT) },
		{ VK_PAUSE,		UCHAR_MAMEKEY(PAUSE) },
		{ VK_CANCEL,	UCHAR_MAMEKEY(CANCEL) }
	};

	if (win_use_natural_keyboard && (message == WM_KEYDOWN))
	{
		for (i = 0; i < sizeof(keytrans) / sizeof(keytrans[0]); i++)
		{
			if (wparam == keytrans[i][0])
			{
				inputx_postc(keytrans[i][1]);
				message = WM_NULL;

				/* check to see if there is a corresponding WM_CHAR in our
				 * future.  If so, remove it
				 */
				PeekMessage(&msg, wnd, 0, 0, PM_NOREMOVE);
				if ((msg.message == WM_CHAR) && (msg.lParam == lparam))
					PeekMessage(&msg, wnd, 0, 0, PM_REMOVE);
				break;
			}
		}
	}

	switch(message)
	{
		case WM_INITMENU:
			prepare_menus(wnd);
			break;

		case WM_CHAR:
			if (win_use_natural_keyboard)
				inputx_postc(wparam);
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

//============================================================
//	osd_keyboard_disabled
//============================================================

int osd_keyboard_disabled(void)
{
	return win_use_natural_keyboard;
}
