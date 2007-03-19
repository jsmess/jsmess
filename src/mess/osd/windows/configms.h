//============================================================
//
//	configms.h - Win32 MESS specific options
//
//============================================================

#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "options.h"

// ugh hack
const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

#endif /* CONFIGMS_H */

