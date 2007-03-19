//============================================================
//
//	dialog.h - Win32 MESS dialog handling
//
//============================================================

#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>
#include "mame.h"
#include "png.h"
#include "opresolv.h"

typedef struct _dialog_box dialog_box;

struct dialog_layout
{
	short label_width;
	short combo_width;
};

typedef void (*dialog_itemstoreval)(void *param, int val);
typedef void (*dialog_itemchangedproc)(dialog_box *dialog, HWND dlgitem, void *changed_param);
typedef void (*dialog_notification)(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *param);

/* dialog allocation / termination */
dialog_box *win_dialog_init(const char *title, const struct dialog_layout *layout);
void win_dialog_exit(dialog_box *dialog);

/* dialog memory allocation */
void *win_dialog_malloc(dialog_box *dialog, size_t size);
char *win_dialog_strdup(dialog_box *dialog, const char *s);
WCHAR *win_dialog_wcsdup(dialog_box *dialog, const WCHAR *s);

#ifdef UNICODE
#define win_dialog_tcsdup	win_dialog_wcsdup
#else
#define win_dialog_tcsdup	win_dialog_strdup
#endif

/* dialog operations */
void win_dialog_runmodal(HWND wnd, dialog_box *dialog);
int win_dialog_add_combobox(dialog_box *dialog, const char *item_label, int default_value,
	dialog_itemstoreval storeval, void *storeval_param);
int win_dialog_add_active_combobox(dialog_box *dialog, const char *item_label, int default_value,
	dialog_itemstoreval storeval, void *storeval_param,
	dialog_itemchangedproc changed, void *changed_param);
int win_dialog_add_combobox_item(dialog_box *dialog, const char *item_label, int item_data);

int win_dialog_add_adjuster(dialog_box *dialog, const char *item_label, int default_value,
	int min_value, int max_value, BOOL is_percentage,
	dialog_itemstoreval storeval, void *storeval_param);

int win_dialog_add_slider(dialog_box *dialog, const char *item_label, int default_value,
	int min_value, int max_value,
	dialog_itemstoreval storeval, void *storeval_param);

int win_dialog_add_portselect(dialog_box *dialog, input_port_entry *port, const RECT *r);

int win_dialog_add_standard_buttons(dialog_box *dialog);
int win_dialog_add_image(dialog_box *dialog, const png_info *png);
int win_dialog_add_separator(dialog_box *dialog);
int win_dialog_add_notification(dialog_box *dialog, UINT notification,
	dialog_notification callback, void *param);


enum file_dialog_type
{
	FILE_DIALOG_OPEN,
	FILE_DIALOG_SAVE
};

// wrapper for the standard file dialog
BOOL win_file_dialog(HWND parent, enum file_dialog_type dlgtype, dialog_box *custom_dialog, const char *filter,
	const char *initial_dir, char *filename, size_t filename_len);

#endif /* DIALOG_H */
