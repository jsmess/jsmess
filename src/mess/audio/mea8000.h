/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Philips MEA 8000 emulation.

**********************************************************************/

#ifndef MEA8000_H
#define MEA8000_H

#define MEA8000 DEVICE_GET_INFO_NAME(mea8000)

/* ---------- configuration ------------ */

typedef struct _mea8000_interface mea8000_interface;
struct _mea8000_interface
{
  /* output channel */
  const char *           channel;

  /* 1-bit 'ready' output, not negated */
  write8_device_func req_out_func;
};


#define MDRV_MEA8000_ADD(_tag, _intrf)	      \
  MDRV_DEVICE_ADD(_tag, MEA8000, 0)	      \
  MDRV_DEVICE_CONFIG(_intrf)


/* ---------- functions ------------ */

DEVICE_GET_INFO(mea8000);

/* interface to CPU via address/data bus*/
extern READ8_DEVICE_HANDLER  ( mea8000_r );
extern WRITE8_DEVICE_HANDLER ( mea8000_w );

#endif
