#ifndef __CBM_SERIAL_BUS_H_
#define __CBM_SERIAL_BUS_H_

#include "cbmdrive.h"

void cbmfloppy_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

#define IEC 1
#define SERIAL 2
#define IEEE 3
void cbm_drive_0_config (int interface, int serialnr);
void cbm_drive_1_config (int interface, int serialnr);

/* delivers status for displaying */
extern void cbm_drive_0_status (char *text, int size);
extern void cbm_drive_1_status (char *text, int size);

/* iec interface c16/c1551 */
void c1551_0_write_data (int data);
int c1551_0_read_data (void);
void c1551_0_write_handshake (int data);
int c1551_0_read_handshake (void);
int c1551_0_read_status (void);

void c1551_1_write_data (int data);
int c1551_1_read_data (void);
void c1551_1_write_handshake (int data);
int c1551_1_read_handshake (void);
int c1551_1_read_status (void);

/* serial bus vc20/c64/c16/vc1541 and some printer */
void cbm_serial_reset_write (int level);
int cbm_serial_atn_read (void);
void cbm_serial_atn_write (int level);
int cbm_serial_data_read (void);
void cbm_serial_data_write (int level);
int cbm_serial_clock_read (void);
void cbm_serial_clock_write (int level);
int cbm_serial_request_read (void);
void cbm_serial_request_write (int level);

/* private */
extern CBM_Drive cbm_drive[2];

#endif
