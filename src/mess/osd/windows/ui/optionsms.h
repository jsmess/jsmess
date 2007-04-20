#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "device.h"
#include "options.h"

enum
{
	MESS_COLUMN_IMAGES,
	MESS_COLUMN_GOODNAME,
	MESS_COLUMN_MANUFACTURER,
	MESS_COLUMN_YEAR,
	MESS_COLUMN_PLAYABLE,
	MESS_COLUMN_CRC,
	MESS_COLUMN_SHA1,
	MESS_COLUMN_MD5,
	MESS_COLUMN_MAX
};

extern const options_entry mess_wingui_settings[];

void SetMessColumnWidths(int widths[]);
void GetMessColumnWidths(int widths[]);

void SetMessColumnOrder(int order[]);
void GetMessColumnOrder(int order[]);

void SetMessColumnShown(int shown[]);
void GetMessColumnShown(int shown[]);

void SetMessSortColumn(int column);
int  GetMessSortColumn(void);

void SetMessSortReverse(BOOL reverse);
BOOL GetMessSortReverse(void);

const char* GetSoftwareDirs(void);
void  SetSoftwareDirs(const char* paths);

void SetHashDirs(const char *dir);
const char *GetHashDirs(void);

void SetSelectedSoftware(int driver_index, const device_class *devclass, int device_inst, const char *software);
const char *GetSelectedSoftware(int driver_index, const device_class *devclass, int device_inst);

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths);
const char *GetExtraSoftwarePaths(int driver_index);

void SetCurrentSoftwareTab(const char *shortname);
const char *GetCurrentSoftwareTab(void);

#endif /* OPTIONSMS_H */

