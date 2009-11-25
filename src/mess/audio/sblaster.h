/*
  8bit analog output
  8bit analog input
  optional dma, irq

  midi serial port

  pro:
  mixer

  16:
  additional 16 bit adc ,dac
*/

#ifndef SBLASTER_H_
#define SBLASTER_H_


/*----------- defined in audio/sblaster.c -----------*/

 READ8_HANDLER( soundblaster_r );
WRITE8_HANDLER( soundblaster_w );

typedef struct {
	int dma;
	int irq;
	struct { UINT8 major, minor; } version;
} SOUNDBLASTER_CONFIG;

void soundblaster_config(const SOUNDBLASTER_CONFIG *config);


#endif /* SBLASTER_H_ */
