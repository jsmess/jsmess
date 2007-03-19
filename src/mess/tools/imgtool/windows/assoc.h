#ifndef ASSOC_H
#define ASSOC_H

#include <windows.h>

struct win_association_info
{
	LPCTSTR file_class;
	int icon_id;
	LPCTSTR open_args;
};


BOOL win_association_exists(const struct win_association_info *assoc);
BOOL win_is_extension_associated(const struct win_association_info *assoc,
	const char *extension);
BOOL win_associate_extension(const struct win_association_info *assoc,
	const char *extension, BOOL is_set);



#endif // ASSOC_H

