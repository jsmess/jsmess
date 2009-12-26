/***************************************************************************

    device.h

    Definitions and manipulations for device structures

***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

// MAME headers
#include "mamecore.h"
#include "devintrf.h"

// MESS headers
#include "osdmess.h"
#include "opresolv.h"
#include "image.h"


/*************************************
 *
 *  Other
 *
 *************************************/

/* device naming */
const char *device_typename(iodevice_t type);
const char *device_brieftypename(iodevice_t type);
iodevice_t device_typeid(const char *name);

#endif /* DEVICE_H */
