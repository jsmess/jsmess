/*****************************************************************************
 *
 * includes/vc20.h
 *
 ****************************************************************************/

#ifndef VC20_H_
#define VC20_H_


#define VC20ADDR2VIC6560ADDR(a) (((a) > 0x8000) ? ((a) & 0x1fff) : ((a) | 0x2000))
#define VIC6560ADDR2VC20ADDR(a) (((a) > 0x2000) ? ((a) & 0x1fff) : ((a) | 0x8000))


/*----------- defined in machine/vc20.c -----------*/

extern UINT8 *vc20_memory_9400;

WRITE8_HANDLER ( vc20_write_9400 );

WRITE8_HANDLER( vc20_0400_w );
WRITE8_HANDLER( vc20_2000_w );
WRITE8_HANDLER( vc20_4000_w );
WRITE8_HANDLER( vc20_6000_w );

/* split for more performance */
/* VIC reads bits 8 till 11 */
int vic6560_dma_read_color (int offset);

/* VIC reads bits 0 till 7 */
int vic6560_dma_read (int offset);

DRIVER_INIT( vc20 );
DRIVER_INIT( vic20 );
DRIVER_INIT( vic20i );
DRIVER_INIT( vic1001 );
DRIVER_INIT( vc20v );
DRIVER_INIT( vic20v );

MACHINE_RESET( vic20 );
INTERRUPT_GEN( vic20_frame_interrupt );

void vic20_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#endif /* VC20_H_ */
