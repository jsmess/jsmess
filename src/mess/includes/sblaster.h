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

#include "driver.h"

 READ8_HANDLER( soundblaster_r );
WRITE8_HANDLER( soundblaster_w );

typedef struct {
	int dma;
	int irq;	
	struct { UINT8 major, minor; } version;
} SOUNDBLASTER_CONFIG;

void soundblaster_config(const SOUNDBLASTER_CONFIG *config);
void soundblaster_reset(void);

//        { SOUND_CUSTOM, &soundblaster_interface },
//extern struct CustomSound_interface soundblaster_interface;
