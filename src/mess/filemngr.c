/*********************************************************************

  filemngr.c

  MESS's clunky built-in file manager

*********************************************************************/

#include "driver.h"
#include "utils.h"
#include "image.h"
#include "ui.h"
#include "uimenu.h"
#include "uitext.h"

#define SEL_BITS	12
#define SEL_MASK	((1<<SEL_BITS)-1)

static int count_chars_entered;
static char *enter_string;
static int enter_string_size;
static int enter_filename_mode;
static char curdir[260];

static char entered_filename[512];

static void start_enter_string(char *string_buffer, int max_string_size, int filename_mode)
{
	enter_string = string_buffer;
	count_chars_entered = strlen(string_buffer);
	enter_string_size = max_string_size;
	enter_filename_mode = filename_mode;
}


/* code, lower case (w/o shift), upper case (with shift), control */
static const int code_to_char_table[] =
{
	KEYCODE_0, '0', ')', 0,
	KEYCODE_1, '1', '!', 0,
	KEYCODE_2, '2', '"', 0,
	KEYCODE_3, '3', '#', 0,
	KEYCODE_4, '4', '$', 0,
	KEYCODE_5, '5', '%', 0,
	KEYCODE_6, '6', '^', 0,
	KEYCODE_7, '7', '&', 0,
	KEYCODE_8, '8', '*', 0,
	KEYCODE_9, '9', '(', 0,
	KEYCODE_A, 'a', 'A', 1,
	KEYCODE_B, 'b', 'B', 2,
	KEYCODE_C, 'c', 'C', 3,
	KEYCODE_D, 'd', 'D', 4,
	KEYCODE_E, 'e', 'E', 5,
	KEYCODE_F, 'f', 'F', 6,
	KEYCODE_G, 'g', 'G', 7,
	KEYCODE_H, 'h', 'H', 8,
	KEYCODE_I, 'i', 'I', 9,
	KEYCODE_J, 'j', 'J', 10,
	KEYCODE_K, 'k', 'K', 11,
	KEYCODE_L, 'l', 'L', 12,
	KEYCODE_M, 'm', 'M', 13,
	KEYCODE_N, 'n', 'N', 14,
	KEYCODE_O, 'o', 'O', 15,
	KEYCODE_P, 'p', 'P', 16,
	KEYCODE_Q, 'q', 'Q', 17,
	KEYCODE_R, 'r', 'R', 18,
	KEYCODE_S, 's', 'S', 19,
	KEYCODE_T, 't', 'T', 20,
	KEYCODE_U, 'u', 'U', 21,
	KEYCODE_V, 'v', 'V', 22,
	KEYCODE_W, 'w', 'W', 23,
	KEYCODE_X, 'x', 'X', 24,
	KEYCODE_Y, 'y', 'Y', 25,
	KEYCODE_Z, 'z', 'Z', 26,
	KEYCODE_OPENBRACE, '[', '{', 27,
	KEYCODE_BACKSLASH, '\\', '|', 28,
	KEYCODE_CLOSEBRACE, ']', '}', 29,
	KEYCODE_TILDE, '^', '~', 30,
	KEYCODE_BACKSPACE, 127, 127, 31,
	KEYCODE_COLON, ':', ';', 0,
	KEYCODE_EQUALS, '=', '+', 0,
	KEYCODE_MINUS, '-', '_', 0,
	KEYCODE_STOP, '.', '<', 0,
	KEYCODE_COMMA, ',', '>', 0,
	KEYCODE_SLASH, '/', '?', 0,
	KEYCODE_ENTER, 13, 13, 13,
	KEYCODE_ESC, 27, 27, 27
};

/*
 * For now I use a lookup table for valid filename characters.
 * Maybe change this for different platforms?
 * Put it to osd_cpu? Make it an osd_... function?
 */
static const char valid_filename_char[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 00-0f */
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 10-1f */
	1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 	/*	!"#$%&'()*+,-./ */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 	/* 0123456789:;<=>? */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* @ABCDEFGHIJKLMNO */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 	/* PQRSTUVWXYZ[\]^_ */
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* `abcdefghijklmno */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 	/* pqrstuvwxyz{|}~	*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 80-8f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 90-9f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* a0-af */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* b0-bf */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* c0-cf */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* d0-df */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* e0-ef */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0		/* f0-ff */
};

static char code_to_ascii(input_code code)
{
	int i;

	for (i = 0; i < (sizeof (code_to_char_table) / (sizeof (int) * 4)); i++)

	{
		if (code_to_char_table[i * 4] == code)
		{
			if (input_code_pressed(KEYCODE_LCONTROL) || input_code_pressed(KEYCODE_RCONTROL))
				return code_to_char_table[i * 4 + 3];
			if (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT))
				return code_to_char_table[i * 4 + 2];
			return code_to_char_table[i * 4 + 1];
		}
	}

	return -1;
}

static char *update_entered_string(void)
{
	input_code code;
	int ascii_char;

	/* get key */
	code = input_code_poll_switches(FALSE);

	/* key was pressed? */
	if (code == INPUT_CODE_INVALID)
		return NULL;

	ascii_char = code_to_ascii(code);

	switch (ascii_char)
	{
		/* char could not be converted to ascii */
	case -1:
		return NULL;

	case 13:	/* Return */
		return enter_string;

	case 25:	/* Ctrl-Y (clear line) */
		count_chars_entered = 0;
		enter_string[count_chars_entered] = '\0';
		break;

	case 27:	/* Escape */
		return NULL;

		/* delete */
	case 127:
		count_chars_entered--;
		if (count_chars_entered < 0)
			count_chars_entered = 0;
		enter_string[count_chars_entered] = '\0';
		break;

		/* got a char - add to string */
	default:
		if (count_chars_entered < enter_string_size)
		{
			if ((enter_filename_mode && valid_filename_char[(unsigned)ascii_char]) ||
				!enter_filename_mode)
			{
				/* store char */
				enter_string[count_chars_entered] = ascii_char;
				/* update count of chars entered */
				count_chars_entered++;
				/* add null to end of string */
				enter_string[count_chars_entered] = '\0';
			}
		}
		break;
	}

	return NULL;
}


char current_filespecification[32] = "*.*";
const char fs_directory[] = "[DIR]";
const char fs_device[] = "[DRIVE]";
const char fs_file[] = "[FILE]";
/*const char fs_archive[] = "[ARCHIVE]"; */

static ui_menu_item *fs_item;
static int *fs_types;
static int *fs_order;
static int fs_chunk;
static int fs_total;
static int fs_insession;

enum {
	FILESELECT_NONE,
	FILESELECT_QUIT,
	FILESELECT_FILESPEC,
	FILESELECT_DEVICE,
	FILESELECT_DIRECTORY,
	FILESELECT_FILE
} FILESELECT_ENTRY_TYPE;


static void fs_free(void)
{
	if (fs_chunk > 0)
	{
		int i;
		/* free duplicated strings of file and directory names */
		for (i = 0; i < fs_total; i++)
		{
			switch(fs_types[i]) {
			case FILESELECT_FILE:
			case FILESELECT_DIRECTORY:
				if (fs_item[i].text != ui_getstring(UI_emptyslot))
					free((char *) fs_item[i].text);
				break;
			}
		}
		free(fs_item);
		free(fs_types);
		free(fs_order);
		fs_chunk = 0;
		fs_total = 0;
	}
}

static int fs_alloc(void)
{
	if (fs_total >= fs_chunk)
	{
		if (fs_chunk)
		{
			fs_chunk += 256;
			logerror("fs_alloc() next chunk (total %d)\n", fs_chunk);
			fs_item = realloc(fs_item, fs_chunk * sizeof(*fs_item));
			fs_types = realloc(fs_types, fs_chunk * sizeof(int));
			fs_order = realloc(fs_order, fs_chunk * sizeof(int));
		}
		else
		{
			fs_chunk = 512;
			logerror("fs_alloc() first chunk %d\n", fs_chunk);
			fs_item = malloc(fs_chunk * sizeof(*fs_item));
			fs_types = malloc(fs_chunk * sizeof(int));
			fs_order = malloc(fs_chunk * sizeof(int));
		}

		/* what do we do if reallocation fails? raise(SIGABRT) seems a way outa here */
		if (!fs_item || !fs_types || !fs_order)
		{
			logerror("failed to allocate fileselect buffers!\n");
			exit(-1);
		}
	}
	
	memset(&fs_item[fs_total], 0, sizeof(fs_item[fs_total]));
	fs_order[fs_total] = fs_total;
	return fs_total++;
}

static int DECL_SPEC fs_compare(const void *p1, const void *p2)
{
	int i1 = *(int *)p1;
	int i2 = *(int *)p2;

	if (fs_types[i1] != fs_types[i2])
		return fs_types[i1] - fs_types[i2];
	return strcmp(fs_item[i1].text, fs_item[i2].text);
}

#define MAX_ENTRIES_IN_MENU ((1<<12)-2)

static void fs_generate_filelist(void)
{
	osd_directory *dir;
	int qsort_start, count, i, n;
	ui_menu_item *tmp_menu_item;
	int *tmp_types;	

	/* just to be safe */
	fs_free();

	/* quit back to main menu option at top */
	n = fs_alloc();
	fs_item[n].text = ui_getstring(UI_quitfileselector);
	fs_types[n] = FILESELECT_QUIT;

	/* insert blank line */
	n = fs_alloc();
	fs_item[n].text = "-";
	fs_types[n] = FILESELECT_NONE;

	/* current directory */
	n = fs_alloc();
	fs_item[n].text = curdir;
	fs_types[n] = FILESELECT_NONE;

	/* blank line */
	n = fs_alloc();
	fs_item[n].text = "-";
	fs_types[n] = FILESELECT_NONE;

	/* file specification */
	n = fs_alloc();
	fs_item[n].text = ui_getstring(UI_filespecification);
	fs_item[n].subtext = current_filespecification;
	fs_types[n] = FILESELECT_FILESPEC;

	/* insert blank line */
	n = fs_alloc();
	fs_item[n].text = "-";
	fs_types[n] = FILESELECT_NONE;

	/* insert empty specifier */
	n = fs_alloc();
	fs_item[n].text = ui_getstring(UI_emptyslot);
	fs_item[n].subtext = "";
	fs_types[n] = FILESELECT_FILE;

	qsort_start = fs_total;

	/* devices first */
	count = osd_num_devices();
	if (count > 0)
	{
		logerror("fs_generate_filelist: %d devices\n", count);
		for (i = 0; i < count; i++)
		{
			if (fs_total >= MAX_ENTRIES_IN_MENU)
				break;
			n = fs_alloc();
			fs_item[n].text = osd_get_device_name(i);
			fs_item[n].subtext = fs_device;
			fs_types[n] = FILESELECT_DEVICE;
		}
	}

	/* directory entries */
	dir = osd_opendir(curdir);
	if (dir)
	{
		const osd_directory_entry *dirent;

		while((dirent = osd_readdir(dir)) != NULL)
		{
			if (fs_total >= MAX_ENTRIES_IN_MENU)
				break;

			switch(dirent->type)
			{
				case ENTTYPE_DIR:
					n = fs_alloc();
					fs_item[n].text = mame_strdup(dirent->name);
					fs_types[n] = FILESELECT_DIRECTORY;
					fs_item[n].subtext = fs_directory;
					break;

				case ENTTYPE_FILE:
					n = fs_alloc();
					fs_item[n].text = mame_strdup(dirent->name);
					fs_types[n] = FILESELECT_FILE;
					fs_item[n].subtext = fs_file;
					break;

				default:
					/* ignore other file types */
					break;
			}
		}
		osd_closedir(dir);
	}

	logerror("fs_generate_filelist: sorting %d entries\n", n - qsort_start);
	qsort(&fs_order[qsort_start], n - qsort_start, sizeof(int), fs_compare);

	tmp_menu_item = malloc(n * sizeof(*tmp_menu_item));
	tmp_types = malloc(n * sizeof(int));

	/* no space to sort? have to leave now... */
	if (!tmp_menu_item || !tmp_types )
		return;

	/* copy items in original order */
	memcpy(tmp_menu_item, fs_item, n * sizeof(*tmp_menu_item));
	memcpy(tmp_types, fs_types, n * sizeof(int));

	for (i = qsort_start; i < n; i++)
	{
		int j = fs_order[i];
		fs_item[i] = tmp_menu_item[j];
		fs_types[i] = tmp_types[j];
	}

	free(tmp_menu_item);
	free(tmp_types);
}

#define UI_SHIFT_PRESSED		(input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT))
#define UI_CONTROL_PRESSED		(input_code_pressed(KEYCODE_LCONTROL) || input_code_pressed(KEYCODE_RCONTROL))
/* and mask to get bits */
#define SEL_BITS_MASK			(~SEL_MASK)

static int fileselect(int selected, const char *default_selection, const char *working_directory)
{
	int sel, total, arrowize;
	int visible;

	sel = selected - 1;

	/* beginning a file manager session */
	if (fs_insession == 0)
	{
		fs_insession = 1;
		strncpyz(curdir, working_directory, MIN(strlen(working_directory)+1, 260));
	}

	/* generate menu? */
	if (fs_total == 0)
	{
		fs_generate_filelist();
	}

	total = fs_total;

	if (total > 0)
	{
		/* make sure it is in range - might go out of range if
		 * we were stepping up and down directories */
		if ((sel & SEL_MASK) >= total)
			sel = (sel & SEL_BITS_MASK) | (total - 1);

		arrowize = 0;
		if (sel < total)
		{
			switch (fs_types[sel])
			{
				/* arrow pointing inwards (arrowize = 1) */
				case FILESELECT_QUIT:
				case FILESELECT_FILE:
					break;

				case FILESELECT_FILESPEC:
				case FILESELECT_DIRECTORY:
				case FILESELECT_DEVICE:
					/* arrow pointing to right -
					 * indicating more available if
					 * selected, or editable */
					arrowize = 2;
					break;
			}
		}

		if (sel & (1 << SEL_BITS))	/* are we waiting for a new key? */
		{
			char *name;

			/* change menu item to show this filename */
			fs_item[sel & SEL_MASK].subtext = current_filespecification;

			/* display the menu */
			ui_menu_draw(fs_item, fs_total, sel & SEL_MASK, NULL);

			/* update string with any keys that are pressed */
			name = update_entered_string();

			/* finished entering filename? */
			if (name)
			{
				/* yes */
				sel &= SEL_MASK;

				/* if no name entered - go back to default all selection */
				if (strlen(name) == 0)
					strcpy(current_filespecification, "*");
				else
					strcpy(current_filespecification, name);
				fs_free();
			}

			return sel + 1;
		}


		ui_menu_draw(fs_item, fs_total, sel, NULL);

		/* borrowed from usrintrf.c */
		visible = 0; //Machine->uiheight / (3 * Machine->uifontheight /2) -1;

		if (input_ui_pressed_repeat(IPT_UI_DOWN, 8))
		{
			if (UI_CONTROL_PRESSED)
			{
				sel = total - 1;
			}
			else
			if (UI_SHIFT_PRESSED)
			{
				sel = (sel + visible - 1);
				sel = MIN(sel,total);
			}
			else
			{
				sel++;
				sel = MIN(sel,total);
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_UP, 8))
		{
			if (UI_CONTROL_PRESSED)
			{
				sel = 1;
			}
			if (UI_SHIFT_PRESSED)
			{
				sel = (sel - visible + 1);
				sel = MAX(0,sel);
			}
			else
			{
				sel--;
				sel = MAX(0,sel);
			}
		}

		if (input_ui_pressed(IPT_UI_SELECT))
		{
			if (sel < SEL_MASK)
			{
				switch (fs_types[sel])
				{
				case FILESELECT_QUIT:
					sel = -1;
					break;

				case FILESELECT_FILESPEC:
					start_enter_string(current_filespecification, 32, 0);

					/* flush keyboard buffer */
					while (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID)
						;

					sel |= 1 << SEL_BITS; /* we'll ask for a key */
					break;

				case FILESELECT_FILE:
					/* copy filename */
					if (fs_item[sel].text == ui_getstring(UI_emptyslot))
					{
						entered_filename[0] = '\0';
					}
					else
					{
						strncpyz(entered_filename, curdir, strlen(curdir)+1);
						strncatz(entered_filename, PATH_SEPARATOR, sizeof(entered_filename) / sizeof(entered_filename[0]));
						strncatz(entered_filename, fs_item[sel].text, sizeof(entered_filename) / sizeof(entered_filename[0]));
					}

					fs_free();
					sel = -3;
					break;

				case FILESELECT_DEVICE:
					strncpyz(curdir, fs_item[sel].text, MIN(strlen(fs_item[sel].text)+1, 260));
					fs_free();
					break;

				case FILESELECT_DIRECTORY:
					if( strcmp(fs_item[sel].text, "..") == 0 )
					{
						char* sep = strrchr(curdir, PATH_SEPARATOR[0]);
						if( sep ) {
							*sep = '\0';
							sep = strrchr(curdir, PATH_SEPARATOR[0]);
							if( sep )
								*(++sep) = '\0';
						}
					}
					else if( strcmp(fs_item[sel].text, ".") == 0 )
					{
						break;
					}
					else {
						strncatz(curdir, fs_item[sel].text, 260);
						strncatz(curdir, PATH_SEPARATOR, 260);
					}
					fs_free();
					break;

				default:
					break;
				}
			}
		}

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}
	else
	{
		sel = -1;
	}

	if (sel == -1 || sel == -3)
		fs_free();

	if (sel == -1 || sel == -2 || sel == -3)
	{
		fs_insession = 0;
	}

	return sel + 1;
}

int filemanager(int selected)
{
	static int previous_sel;
	const char *name;
	ui_menu_item menu_items[40];
	const struct IODevice *devices[40];
	int ids[40];
	char names[40][64];
	int sel, total, arrowize, id;
	const struct IODevice *dev;
	mess_image *image;

	sel = selected - 1;
	total = 0;

	/* Cycle through all devices for this system */
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		for (id = 0; id < dev->count; id++)
		{
			image = image_from_device_and_index(dev, id);
			strcpy( names[total], image_typename_id(image) );
			name = image_filename(image);

			memset(&menu_items[total], 0, sizeof(menu_items[total]));
			menu_items[total].text = (names[total]) ? names[total] : "---";
			menu_items[total].subtext = (name) ? name : "---";

			devices[total] = dev;
			ids[total] = id;

			total++;
		}
	}


	/* if the fileselect() mode is active */
	if (sel & (2 << SEL_BITS))
	{
		image = image_from_device_and_index(devices[previous_sel & SEL_MASK], ids[previous_sel & SEL_MASK]);
		sel = fileselect(selected & ~(2 << SEL_BITS), image_filename(image), image_working_directory(image));
		if (sel != 0 && sel != -1 && sel!=-2)
			return sel | (2 << SEL_BITS);

		if (sel==-2)
		{
			/* selected a file */

			/* finish entering name */
			previous_sel = previous_sel & SEL_MASK;

			/* attempt a filename change */
			image = image_from_device_and_index(devices[previous_sel], ids[previous_sel]);
			if (entered_filename[0])
				image_load(image, entered_filename);
			else
				image_unload(image);
		}

		sel = previous_sel;

		/* change menu item to show this filename */
		menu_items[sel & SEL_MASK].subtext = entered_filename;
	}

	memset(&menu_items[total], 0, sizeof(menu_items[total]));
	menu_items[total].text = ui_getstring(UI_returntomain);
	total++;

	arrowize = 0;
	if (sel < total - 1)
		arrowize = 2;

	if (sel & (1 << SEL_BITS))	/* are we waiting for a new key? */
	{
		/* change menu item to show this filename */
		menu_items[sel & SEL_MASK].subtext = entered_filename;

		/* display the menu */
		ui_menu_draw(menu_items, total, sel & SEL_MASK, NULL);

		/* update string with any keys that are pressed */
		name = update_entered_string();

		/* finished entering filename? */
		if (name)
		{
			/* yes */
			sel &= SEL_MASK;
			image = image_from_device_and_index(devices[sel], ids[sel]);
			image_load(image, NULL);
		}

		return sel + 1;
	}

	ui_menu_draw(menu_items, total, sel, NULL);

	if (input_ui_pressed_repeat(IPT_UI_DOWN, 8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP, 8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		int os_sel;

		image = NULL;

		/* Return to main menu? */
		if (sel == total-1)
		{
			sel = -1;
			os_sel = -1;
		}
		/* no, let the osd code have a crack at changing files */
		else
		{
			image = image_from_device_and_index(devices[sel], ids[sel]);
			os_sel = 0;
		}

		if (os_sel != 0)
		{
			if (os_sel == 1)
			{
				/* attempt a filename change */
				image_load(image, entered_filename);
			}
		}
		/* osd code won't handle it, lets use our clunky interface */
		else if (!UI_SHIFT_PRESSED)
		{
			/* save selection and switch to fileselect() */
			previous_sel = sel;
			sel = (2 << SEL_BITS);
			fs_total = 0;
		}
		else
		{
			if (strcmp(menu_items[sel].text, "---") == 0)
				entered_filename[0] = '\0';
			else
				strcpy(entered_filename, menu_items[sel].text);
			start_enter_string(entered_filename, (sizeof(entered_filename) / sizeof(entered_filename[0])) - 1, 1);

			/* flush keyboard buffer */
			while (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID)
				;

			sel |= 1 << SEL_BITS;	/* we'll ask for a key */
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	return sel + 1;
}

