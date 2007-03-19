#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "device.h"

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

struct mess_specific_options
{
	BOOL   use_new_ui;
	UINT32 ram_size;
	char   *software[64];
};

struct mess_specific_game_variables
{
	char *extra_software_paths;
};

struct mess_specific_settings
{
	int      mess_column_width[MESS_COLUMN_MAX];
	int      mess_column_order[MESS_COLUMN_MAX];
	int      mess_column_shown[MESS_COLUMN_MAX];

	int      mess_sort_column;
	BOOL     mess_sort_reverse;

	char*    softwaredirs;
	char*    hashdir;

	char*    software_tab;
};

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

void SetCrcDir(const char *dir);
const char *GetCrcDir(void);

void SetSelectedSoftware(int driver_index, int device_inst_index, const char *software);
const char *GetSelectedSoftware(int driver_index, int device_inst_index);

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths);
const char *GetExtraSoftwarePaths(int driver_index);

void SetCurrentSoftwareTab(const char *shortname);
const char *GetCurrentSoftwareTab(void);

BOOL LoadDeviceOption(DWORD nSettingsFile, char *key, const char *value_str);
void SaveDeviceOption(DWORD nSettingsFile, void (*emit_callback)(void *param_, const char *key, const char *value_str, const char *comment), void *param);

#endif /* OPTIONSMS_H */

