//============================================================
//
//	configms.h - Win32 MESS specific options
//
//============================================================

#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "options.h"

extern int win_write_config;

void win_mess_extract_options(void);

int write_config (const char* filename, const game_driver *gamedrv);
const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

void win_add_mess_device_options(const game_driver *gamedrv);
void win_mess_driver_name_callback(const char *arg);
void win_mess_config_init(running_machine *machine);

#endif /* CONFIGMS_H */

