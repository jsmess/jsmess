/* Kermit protocol implementation.

   Transfer between an emulated machine and an image using the kermit protocol.

   Used in the HP48 S/SX/G/GX emulation.

   Author: Antoine Mine'
   Date: 29/03/2008
 */

#include "timer.h"
#include "image.h"


#define KERMIT DEVICE_GET_INFO_NAME( kermit )


typedef struct {

  /* called by Kermit when it wants to send a byte to the emulated machine */
  void (*send)( running_machine *machine, UINT8 data );

} kermit_config;


#define MDRV_KERMIT_ADD(_tag, _intrf) \
  MDRV_DEVICE_ADD(_tag, KERMIT, 0)	      \
  MDRV_DEVICE_CONFIG(_intrf)


extern DEVICE_GET_INFO( kermit );


/* call this when the emulated machine has read the last byte sent by
   Kermit through the send call-back */
extern void kermit_byte_transmitted( running_device *device );

/* call this when the emulated machine sends a byte to Kermit */
extern void kermit_receive_byte( running_device *device, UINT8 data );
