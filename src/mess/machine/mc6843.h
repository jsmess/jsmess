/**********************************************************************

  Copyright (C) Antoine Mine' 2007

  Motorola 6843 Floppy Disk Controller emulation.

**********************************************************************/

#ifndef MC6843_H
#define MC6843_H

#define MC6843 DEVICE_GET_INFO_NAME(mc6843)


/* ---------- configuration ------------ */

typedef struct _mc6843_interface mc6843_interface;
struct _mc6843_interface
{
	void ( * irq_func ) ( running_device *device, int state );
};


#define MDRV_MC6843_ADD(_tag, _intrf) \
  MDRV_DEVICE_ADD(_tag, MC6843, 0)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MC6843_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)


/* ---------- functions ------------ */

extern DEVICE_GET_INFO(mc6843);

extern READ8_DEVICE_HANDLER  ( mc6843_r );
extern WRITE8_DEVICE_HANDLER ( mc6843_w );

extern void mc6843_set_drive ( running_device *device, int drive );
extern void mc6843_set_side  ( running_device *device, int side );
extern void mc6843_set_index_pulse ( running_device *device, int index_pulse );

#endif
