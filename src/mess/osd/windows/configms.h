//============================================================
//
//	configms.h - Win32 MESS specific options
//
//============================================================

#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "options.h"


//============================================================
//	GLOBAL VARIABLES
//============================================================

extern const options_entry mess_win_options[];


//============================================================
//	UGH HACK
//============================================================

const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

#endif /* CONFIGMS_H */

