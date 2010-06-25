/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6854 emulation (network interface).

**********************************************************************/

#ifndef MC6854_H
#define MC6854_H

DECLARE_LEGACY_DEVICE(MC6854, mc6854);

/* we provide two interfaces:
   - a bit-based interface:   out_tx, set_rx
   - a frame-based interface: out_frame, send_frame

   The bit-based interface is low-level and slow.
   Use it to simulate the actual bits sent into the wires, e.g., to connect
   the emulator to another bit-based emulated network device, or an actual
   device.

   The frame-based interface is higher-level and faster.
   It passes bytes directly from one end to the other without bothering with
   the actual bit-encoding, synchronization, and CRC.
   Once completed, a frame is sent through out_frame. Aborted frames are not
   transmitted at all. No start flag, stop flag, or crc bits are trasmitted.
   send_frame makes a frame available to the CPU through the 6854 (it may
   fail and return -1 if the 6854 is not ready to accept the frame; even
   if the frame is accepted and 0 is returned, the CPU may abort it). Ony
   full frames are accepted.
*/


/* ---------- configuration ------------ */

typedef struct _mc6854_interface mc6854_interface;
struct _mc6854_interface
{
  /* low-level, bit-based interface */
  void ( * out_tx  ) ( running_device *device, int state ); /* transmit bit */

  /* high-level, frame-based interface */
  void ( * out_frame ) ( running_device *device, UINT8* data, int length );

  /* control lines */
  void ( * out_rts ) ( running_device *device, int state ); /* 1 = transmitting, 0 = idle */
  void ( * out_dtr ) ( running_device *device, int state ); /* 1 = data transmit ready, 0 = busy */
};


#define MDRV_MC6854_ADD(_tag, _intrf) \
  MDRV_DEVICE_ADD(_tag, MC6854, 0)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_MC6854_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)


/* ---------- functions ------------ */
/* interface to CPU via address/data bus*/
extern READ8_DEVICE_HANDLER  ( mc6854_r );
extern WRITE8_DEVICE_HANDLER ( mc6854_w );

/* low-level, bit-based interface */
extern void mc6854_set_rx ( running_device *device, int state );

/* high-level, frame-based interface */
extern int mc6854_send_frame( running_device *device, UINT8* data, int length ); /* ret -1 if busy */

/* control lines */
extern void mc6854_set_cts ( running_device *device, int state ); /* 1 = clear-to-send, 0 = busy */
extern void mc6854_set_dcd ( running_device *device, int state ); /* 1 = carrier, 0 = no carrier */

#endif
