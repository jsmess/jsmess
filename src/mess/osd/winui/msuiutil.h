#ifndef MSUIUTIL_H
#define MSUIUTIL_H

BOOL DriverIsComputer(int driver_index);
BOOL DriverIsConsole(int driver_index);
BOOL DriverIsModified(int driver_index);
BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type);

#endif /* MSUIUTIL_H */

