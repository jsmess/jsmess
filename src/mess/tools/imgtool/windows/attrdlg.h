#include <windows.h>
#include "../imgtool.h"

struct transfer_suggestion_info
{
	int selected;
	imgtool_transfer_suggestion suggestions[8];
};

UINT_PTR CALLBACK win_new_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam);

imgtoolerr_t win_show_option_dialog(HWND parent, struct transfer_suggestion_info *suggestion_info,
	const struct OptionGuide *guide, const char *optspec, option_resolution **result, BOOL *cancel);

