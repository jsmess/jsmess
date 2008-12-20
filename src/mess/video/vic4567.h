/*****************************************************************************
 *
 * video/vic4567.h
 *
 ****************************************************************************/

#ifndef VIC4567_H_
#define VIC4567_H_


/*----------- defined in video/vic4567.c -----------*/

extern const unsigned char vic3_palette[0x100 * 3];

extern void vic4567_init (running_machine *machine, int pal, int (*dma_read)(running_machine *, int),
						  int (*dma_read_color)(running_machine *, int), void (*irq)(running_machine *, int),
						  void (*param_port_changed)(running_machine *, int));

extern INTERRUPT_GEN( vic3_raster_irq );

/* to be called when writting to port */
extern WRITE8_HANDLER ( vic3_port_w );
WRITE8_HANDLER( vic3_palette_w );

/* to be called when reading from port */
extern  READ8_HANDLER ( vic3_port_r );


#endif /* VIC4567_H_ */
