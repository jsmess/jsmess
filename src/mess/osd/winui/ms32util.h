#ifndef MS32UTIL_H
#define MS32UTIL_H

#include <windows.h>

BOOL DriverIsComputer(int driver_index);
BOOL DriverIsModified(int driver_index);
BOOL DriverUsesMouse(int driver_index);
BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type);

#endif /* MS32UTIL_H */

