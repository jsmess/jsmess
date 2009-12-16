//============================================================
//
//  configms.h - Win32 MESS specific options
//
//============================================================

#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "options.h"

extern int win_write_config;

void osd_mess_config_init(running_machine *machine);
void osd_mess_options_init(core_options *opts);

// ugh hack
const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

#endif /* CONFIGMS_H */

