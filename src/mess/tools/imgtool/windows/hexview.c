//============================================================
//
//	hexview.c - A Win32 hex editor control
//
//============================================================

#include <stdio.h>
#include <tchar.h>
#include "hexview.h"
#include "utils.h"
#include "winutils.h"

const TCHAR hexview_class[] = "hexview_class";



struct hexview_info
{
	void *data;
	size_t data_size;
	int index_width;
	int left_margin;
	int right_margin;
	int byte_spacing;
	HFONT font;
};



static struct hexview_info *get_hexview_info(HWND hexview)
{
	LONG_PTR l;
	l = GetWindowLongPtr(hexview, GWLP_USERDATA);
	return (struct hexview_info *) l;
}



static void calc_hexview_bytesperrow(HWND hexview, const TEXTMETRIC *metrics,
	LONG *bytes_per_row)
{
	RECT r;
	struct hexview_info *info;
	LONG width;
	SCROLLINFO si;

	info = get_hexview_info(hexview);
	GetWindowRect(hexview, &r);
	width = r.right - r.left;

	// compensate for scroll bar
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
	GetScrollInfo(hexview, SB_VERT, &si);
	if (si.nPos + si.nPage < si.nMax)
		width -= GetSystemMetrics(SM_CXHSCROLL);

	*bytes_per_row = (width - info->left_margin - info->right_margin - metrics->tmMaxCharWidth * info->index_width)
		/ (metrics->tmMaxCharWidth * (3 + info->byte_spacing));
	*bytes_per_row = MAX(*bytes_per_row, 1);
}



static void hexview_paint(HWND hexview)
{
	struct hexview_info *info;
	HDC dc;
	PAINTSTRUCT ps;
	LONG bytes_per_row;
	LONG begin_row, end_row, row, col, pos, scroll_y;
	TEXTMETRIC metrics;
	TCHAR buf[32];
	TCHAR offset_format[8];
	RECT r;
	BYTE b;
	SCROLLINFO si;

	info = get_hexview_info(hexview);
	dc = BeginPaint(hexview, &ps);

	if (info->font)
		SelectObject(dc, info->font);
	SetBkColor(dc, GetSysColor(COLOR_WINDOW));

	_sntprintf(offset_format, sizeof(offset_format) / sizeof(offset_format[0]),
		TEXT("%%0%dX"), info->index_width);

	// figure out how many bytes go in one row
	GetTextMetrics(dc, &metrics);
	calc_hexview_bytesperrow(hexview, &metrics, &bytes_per_row);

	// compensate for scrolling
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hexview, SB_VERT, &si);
	scroll_y = si.nPos;

	// draw the relevant rows
	begin_row = (ps.rcPaint.top + scroll_y) / metrics.tmHeight;
	end_row = (ps.rcPaint.bottom + scroll_y - 1) / metrics.tmHeight;
	for (row = begin_row; row <= end_row; row++)
	{
		_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), offset_format, row * bytes_per_row);

		r.top = row * metrics.tmHeight - scroll_y;
		r.left = 0;
		r.bottom = r.top + metrics.tmHeight;
		r.right = r.left + metrics.tmMaxCharWidth * info->index_width;

		DrawText(dc, buf, -1, &r, DT_LEFT);

		for (col = 0; col < bytes_per_row; col++)
		{
			pos = row * bytes_per_row + col;
			if (pos < info->data_size)
			{
				b = ((BYTE *) info->data)[pos];

				r.left = (info->index_width + col * (2 + info->byte_spacing)) * metrics.tmMaxCharWidth
					+ info->left_margin;
				r.right = r.left + metrics.tmMaxCharWidth * 2;
				_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%02X"), b);
				DrawText(dc, buf, -1, &r, DT_LEFT);

				r.left = (info->index_width + bytes_per_row * (2 + info->byte_spacing) + col) * metrics.tmMaxCharWidth
					+ info->left_margin + info->right_margin;
				r.right = r.left + metrics.tmMaxCharWidth * 2;
				buf[0] = ((b >= 0x20) && (b <= 0x7F)) ? (TCHAR) b : '.';
				DrawText(dc, buf, 1, &r, DT_LEFT);
			}
		}
	}

	EndPaint(hexview, &ps);
}



static void calc_scrollbar(HWND hexview)
{
	struct hexview_info *info;
	HDC dc;
	RECT r;
	LONG bytes_per_row, absolute_rows, page_rows;
	TEXTMETRIC metrics;
	SCROLLINFO si;

	info = get_hexview_info(hexview);
	dc = GetDC(hexview);
	if (info->font)
		SelectObject(dc, info->font);
	GetTextMetrics(dc, &metrics);
	GetWindowRect(hexview, &r);

	calc_hexview_bytesperrow(hexview, &metrics, &bytes_per_row);

	absolute_rows = (info->data_size + bytes_per_row - 1) / bytes_per_row;
	page_rows = (r.bottom - r.top) / metrics.tmHeight;

	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE;
	si.nMin = 0;
	si.nMax = absolute_rows * metrics.tmHeight;
	si.nPage = page_rows * metrics.tmHeight;
	SetScrollInfo(hexview, SB_VERT, &si, FALSE);

	ReleaseDC(hexview, dc);
}



static int hexview_create(HWND hexview, CREATESTRUCT *cs)
{
	struct hexview_info *info;

	info = (struct hexview_info *) malloc(sizeof(*info));
	if (!info)
		return -1;
	memset(info, '\0', sizeof(*info));

	info->index_width = 4;
	info->left_margin = 8;
	info->right_margin = 8;
	info->byte_spacing = 1;

	SetWindowLongPtr(hexview, GWLP_USERDATA, (LONG_PTR) info);
	calc_scrollbar(hexview);
	return 0;
}



static LRESULT CALLBACK hexview_wndproc(HWND hexview, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct hexview_info *info;

	info = get_hexview_info(hexview);
	
	switch(message)
	{
		case WM_CREATE:
			if (hexview_create(hexview, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			if (info->data)
				free(info->data);
			free(info);
			break;

		case WM_PAINT:
			hexview_paint(hexview);
			break;

		case WM_SIZE:
			calc_scrollbar(hexview);
			InvalidateRect(hexview, NULL, TRUE);
			break;

		case WM_SETFONT:
			info->font = (HFONT) wparam;
			if (lparam)
				RedrawWindow(hexview, NULL, NULL, 0);
			return 0;

		case WM_GETFONT:
			return (LRESULT) info->font;

		case WM_VSCROLL:
			win_scroll_window(hexview, wparam, SB_VERT, 10);
			break;
	}
	return DefWindowProc(hexview, message, wparam, lparam);
}



BOOL hexview_setdata(HWND hexview, const void *data, size_t data_size)
{
	struct hexview_info *info;
	void *data_copy;

	info = get_hexview_info(hexview);

	if ((data_size != info->data_size) || memcmp(data, info->data, data_size))
	{
		data_copy = malloc(data_size);
		if (!data_copy)
			return FALSE;
		memcpy(data_copy, data, data_size);

		if (info->data)
			free(info->data);
		info->data = data_copy;
		info->data_size = data_size;

		calc_scrollbar(hexview);
		InvalidateRect(hexview, NULL, TRUE);
	}
	return TRUE;
}



BOOL hexview_registerclass()
{
	WNDCLASS hexview_wndclass;

	memset(&hexview_wndclass, 0, sizeof(hexview_wndclass));
	hexview_wndclass.lpfnWndProc = hexview_wndproc;
	hexview_wndclass.lpszClassName = hexview_class;
	hexview_wndclass.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	return RegisterClass(&hexview_wndclass);
}

