/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6846 timer emulation.

**********************************************************************/

#ifndef MC6846_H
#define MC6846_H

#define MC6846 DEVICE_GET_INFO_NAME(mc6846)

/* ---------- configuration ------------ */

typedef struct _mc6846_interface mc6846_interface;
struct _mc6846_interface
{
  /* CPU write to the outside through chip */
  write8_device_func out_port_func;  /* 8-bit output */
  write8_device_func out_cp1_func;   /* 1-bit output */
  write8_device_func out_cp2_func;   /* 1-bit output */

  /* CPU read from the outside through chip */
  read8_device_func in_port_func; /* 8-bit input */

  /* asynchronous timer output to outside world */
  write8_device_func out_cto_func; /* 1-bit output */

  /* timer interrupt */
  void ( * irq_func ) ( running_device *device, int state );
};


#define MDRV_MC6846_ADD(_tag, _intrf) \
  MDRV_DEVICE_ADD(_tag, MC6846, 0)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MC6846_MODIFY(_tag, _intrf) \
  MDRV_DEVICE_MODIFY(_tag)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MC6846_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)


/* ---------- functions ------------ */


extern DEVICE_GET_INFO(mc6846);

/* interface to CPU via address/data bus*/
extern READ8_DEVICE_HANDLER  ( mc6846_r );
extern WRITE8_DEVICE_HANDLER ( mc6846_w );

/* asynchronous write from outside world into interrupt-generating pins */
extern void mc6846_set_input_cp1 ( running_device *device, int data );
extern void mc6846_set_input_cp2 ( running_device *device, int data );

/* polling from outside world */
extern UINT8  mc6846_get_output_port ( running_device *device );
extern UINT8  mc6846_get_output_cto  ( running_device *device );
extern UINT8  mc6846_get_output_cp2  ( running_device *device );

/* partial access to internal state */
extern UINT16 mc6846_get_preset ( running_device *device ); /* timer interval - 1 in us */

#endif

