#ifndef __VIC4567_H_
#define __VIC4567_H_

#include "vic6567.h"

extern unsigned char vic3_palette[0x100 * 3];

extern void vic4567_init (int pal, int (*dma_read) (int),
						  int (*dma_read_color) (int), void (*irq) (int),
						  void (*param_port_changed)(int));

extern INTERRUPT_GEN( vic3_raster_irq );

/* to be called when writting to port */
extern WRITE8_HANDLER ( vic3_port_w );
WRITE8_HANDLER( vic3_palette_w );

/* to be called when reading from port */
extern  READ8_HANDLER ( vic3_port_r );


#endif

