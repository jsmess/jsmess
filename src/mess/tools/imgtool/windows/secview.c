//============================================================
//
//	secview.c - A Win32 sector editor dialog
//
//============================================================

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>

#include "secview.h"
#include "wimgres.h"
#include "hexview.h"

#define ANCHOR_LEFT		0x01
#define ANCHOR_TOP		0x02
#define ANCHOR_RIGHT	0x04
#define ANCHOR_BOTTOM	0x08

struct anchor_entry
{
	UINT16 control;
	UINT8 anchor;
};

struct sectorview_info
{
	imgtool_image *image;
	HFONT font;
	UINT32 track, head, sector;
	LONG old_width;
	LONG old_height;
};

static const struct anchor_entry sectorview_anchor[] =
{
	{ IDC_HEXVIEW,		ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT | ANCHOR_BOTTOM },
	{ IDOK,				ANCHOR_RIGHT | ANCHOR_BOTTOM },
	{ IDC_TRACKEDIT,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_TRACKLABEL,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_TRACKSPIN,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_HEADEDIT,		ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_HEADLABEL,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_HEADSPIN,		ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_SECTOREDIT,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_SECTORLABEL,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ IDC_SECTORSPIN,	ANCHOR_LEFT | ANCHOR_BOTTOM },
	{ 0 }
};



static struct sectorview_info *get_sectorview_info(HWND dialog)
{
	LONG_PTR l;
	l = GetWindowLongPtr(dialog, GWLP_USERDATA);
	return (struct sectorview_info *) l;
}



static HFONT create_font(void)
{
	HDC temp_dc;
	HFONT font;

	temp_dc = GetDC(NULL);

	font = CreateFont(-MulDiv(8, GetDeviceCaps(temp_dc, LOGPIXELSY), 72), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
				ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("Lucida Console"));
	if (temp_dc)
		ReleaseDC(NULL, temp_dc);
	return font;
}



static void size_dialog(HWND dialog, const struct anchor_entry *anchor_entries,
	LONG width_delta, LONG height_delta)
{
	int i;
	HWND dlgitem;
	RECT dialog_rect, adjusted_dialog_rect, dlgitem_rect;
	LONG dialog_left, dialog_top;
	LONG left, top, width, height;
	UINT8 anchor;
	HANDLE width_prop, height_prop;
	static const TCHAR winprop_negwidth[] = TEXT("winprop_negwidth");
	static const TCHAR winprop_negheight[] = TEXT("winprop_negheight");

	// figure out the dialog client top/left coordinates
	GetWindowRect(dialog, &dialog_rect);
	adjusted_dialog_rect = dialog_rect;
	AdjustWindowRectEx(&adjusted_dialog_rect,
		GetWindowLong(dialog, GWL_STYLE),
		GetMenu(dialog) ? TRUE : FALSE,
		GetWindowLong(dialog, GWL_EXSTYLE));
	dialog_left = dialog_rect.left + (dialog_rect.left - adjusted_dialog_rect.left);
	dialog_top = dialog_rect.top + (dialog_rect.top - adjusted_dialog_rect.top);

	for (i = 0; anchor_entries[i].control; i++)
	{
		dlgitem = GetDlgItem(dialog, anchor_entries[i].control);
		if (dlgitem)
		{
			GetWindowRect(dlgitem, &dlgitem_rect);
			anchor = anchor_entries[i].anchor;

			left = dlgitem_rect.left - dialog_left;
			top = dlgitem_rect.top - dialog_top;
			width = dlgitem_rect.right - dlgitem_rect.left;
			height = dlgitem_rect.bottom - dlgitem_rect.top;

			width_prop = GetProp(dlgitem, winprop_negwidth);
			if (width_prop)
				width = (LONG) width_prop;
			height_prop = GetProp(dlgitem, winprop_negheight);
			if (height_prop)
				height = (LONG) height_prop;

			switch(anchor & (ANCHOR_LEFT | ANCHOR_RIGHT))
			{
				case ANCHOR_RIGHT:
					left += width_delta;
					break;

				case ANCHOR_LEFT | ANCHOR_RIGHT:
					width += width_delta;
					break;
			}

			switch(anchor & (ANCHOR_TOP | ANCHOR_BOTTOM))
			{
				case ANCHOR_BOTTOM:
					top += height_delta;
					break;

				case ANCHOR_TOP | ANCHOR_BOTTOM:
					height += height_delta;
					break;
			}

			// Record the width/height properties if they are negative
			if (width < 0)
			{
				SetProp(dlgitem, winprop_negwidth, (HANDLE) width);
				width = 0;
			}
			else if (width_prop)
				RemoveProp(dlgitem, winprop_negwidth);
			if (height < 0)
			{
				SetProp(dlgitem, winprop_negheight, (HANDLE) height);
				height = 0;
			}
			else if (height_prop)
				RemoveProp(dlgitem, winprop_negheight);

			// Actually move the window
			SetWindowPos(dlgitem, 0, left, top, width, height, SWP_NOZORDER);
			InvalidateRect(dlgitem, NULL, TRUE);
		}
	}
}



// reads sector data from the disk image into the view
static imgtoolerr_t read_sector_data(HWND dialog, UINT32 track, UINT32 head, UINT32 sector)
{
	imgtoolerr_t err;
	struct sectorview_info *info;
	UINT32 length;
	void *data;

	info = get_sectorview_info(dialog);

	err = imgtool_image_get_sector_size(info->image, track, head, sector, &length);
	if (err)
		goto done;

	data = alloca(length);
	err = imgtool_image_read_sector(info->image, track, head, sector, data, length);
	if (err)
		goto done;

	if (!hexview_setdata(GetDlgItem(dialog, IDC_HEXVIEW), data, length))
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	info->track = track;
	info->head = head;
	info->sector = sector;

done:
	return err;
}



// sets the sector text
static void set_sector_text(HWND dialog)
{
	struct sectorview_info *info;
	TCHAR buf[32];

	info = get_sectorview_info(dialog);

	if (info->track != ~0)
		_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%u"), (unsigned int) info->track);
	else
		buf[0] = '\0';
	SetWindowText(GetDlgItem(dialog, IDC_TRACKEDIT), buf);

	if (info->head != ~0)
		_snprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%u"), (unsigned int) info->head);
	else
		buf[0] = '\0';
	SetWindowText(GetDlgItem(dialog, IDC_HEADEDIT), buf);

	if (info->sector != ~0)
		_snprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%u"), (unsigned int) info->sector);
	else
		buf[0] = '\0';
	SetWindowText(GetDlgItem(dialog, IDC_SECTOREDIT), buf);
}



static void change_sector(HWND dialog)
{
	struct sectorview_info *info;
	imgtoolerr_t err;
	TCHAR buf[32];
	UINT32 new_track, new_head, new_sector;

	info = get_sectorview_info(dialog);

	GetWindowText(GetDlgItem(dialog, IDC_TRACKEDIT), buf, sizeof(buf) / sizeof(buf[0]));
	new_track = (UINT32) _ttoi(buf);
	GetWindowText(GetDlgItem(dialog, IDC_HEADEDIT), buf, sizeof(buf) / sizeof(buf[0]));
	new_head = (UINT32) _ttoi(buf);
	GetWindowText(GetDlgItem(dialog, IDC_SECTOREDIT), buf, sizeof(buf) / sizeof(buf[0]));
	new_sector = (UINT32) _ttoi(buf);

	if ((info->track != new_track) || (info->head != new_head) || (info->sector != new_sector))
	{
		err = read_sector_data(dialog, new_track, new_head, new_sector);
		if (err)
			set_sector_text(dialog);
	}
}



static void setup_spin_control(HWND dialog, int spin_item, int edit_item)
{
	HWND spin_control, edit_control;
	spin_control = GetDlgItem(dialog, spin_item);
	edit_control = GetDlgItem(dialog, edit_item);
	SendMessage(spin_control, UDM_SETBUDDY, (WPARAM) edit_control, 0);
	SendMessage(spin_control, UDM_SETRANGE, 0, MAKELONG(32767, 0));
}



static INT_PTR CALLBACK win_sectorview_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	imgtoolerr_t err;
	INT_PTR rc = 0;
	struct sectorview_info *info;
	RECT dialog_rect;

	switch(message)
	{
		case WM_INITDIALOG:
			info = (struct sectorview_info *) lparam;
			info->font = create_font();

			GetWindowRect(dialog, &dialog_rect);
			info->old_width = dialog_rect.right - dialog_rect.left;
			info->old_height = dialog_rect.bottom - dialog_rect.top;
			info->track = ~0;
			info->head = ~0;
			info->sector = ~0;

			SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_SETFONT, (WPARAM) info->font, (LPARAM) TRUE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);

			setup_spin_control(dialog, IDC_TRACKSPIN, IDC_TRACKEDIT);
			setup_spin_control(dialog, IDC_HEADSPIN, IDC_HEADEDIT);
			setup_spin_control(dialog, IDC_SECTORSPIN, IDC_SECTOREDIT);

			err = read_sector_data(dialog, 0, 0, 0);
			if (err == IMGTOOLERR_SEEKERROR)
				err = read_sector_data(dialog, 0, 0, 1);
			if (!err)
				set_sector_text(dialog);
			break;

		case WM_DESTROY:
			info = get_sectorview_info(dialog);
			if (info->font)
			{
				DeleteObject(info->font);
				info->font = NULL;
			}
			break;

		case WM_SYSCOMMAND:
			if (wparam == SC_CLOSE)
				EndDialog(dialog, 0);
			break;

		case WM_COMMAND:
			switch(HIWORD(wparam))
			{
				case BN_CLICKED:
					switch(LOWORD(wparam))
					{
						case IDOK:
						case IDCANCEL:
							EndDialog(dialog, 0);
							break;
					}
					break;

				case EN_CHANGE:
					change_sector(dialog);
					break;
			}
			break;

		case WM_SIZE:
			info = get_sectorview_info(dialog);

			GetWindowRect(dialog, &dialog_rect);
			size_dialog(dialog, sectorview_anchor,
				(dialog_rect.right - dialog_rect.left) - info->old_width,
				(dialog_rect.bottom - dialog_rect.top) - info->old_height);

			info->old_width = dialog_rect.right - dialog_rect.left;
			info->old_height = dialog_rect.bottom - dialog_rect.top;
			break;

		case WM_MOUSEWHEEL:
			if (HIWORD(wparam) & 0x8000)
				SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_VSCROLL, SB_LINEDOWN, 0);
			else
				SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_VSCROLL, SB_LINEUP, 0);
			break;
	}

	return rc;
}



void win_sectorview_dialog(HWND parent, imgtool_image *image)
{
	struct sectorview_info info;
	
	memset(&info, 0, sizeof(info));
	info.image = image;

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SECTORVIEW), parent,
		win_sectorview_dialog_proc, (LPARAM) &info);
}
