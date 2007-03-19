/***************************************************************************

    sndhrdw/sblaster.c

	Soundblaster code

****************************************************************************/

#include "includes/sblaster.h"
#include "sound/dac.h"

/* operation modes
   0x10 output 8 bit direct
   0x14 output 8 bit with dma
   0x16 output 2 bit compression with dma
   0x17 output 2 bit compression with dma and reference byte
   0x74 output 4 bit compression with dma
   0x75 output 4 bit compression with dma with reference byte
   0x76 output 2.6 bit compression with dma
   0x77 output 2.7 bit compression with dma and reference byte

   0x20 input 8 bit direct
   0x24 input 8 bit with dma
   
   0xd1 speaker on
   0xd3 speaker off
   0xd8 read speaker
   
   0x40 samplerate adjust

   0x48 blockgroesse einstellen

   0x80 interrupt after xx samples

   0xd0 end dma
   0xd4 continue dma

   0xe1 read version

   0x30 midi input
   0x31 midi input with irq generation
   0x32 midi input with time stamp
   0x33 midi input with irq and time stamp
   0x34 midi output mode
   0x35 midi output with irq
   0x37 midi output with time stamp and irq
   0x38 midi output direct

   pro
   0x48, 0x91 output 8 bit high speed 8bit with dma
   0x48, 0x99 input 8 bit high speed 8bit with dma
*/
typedef enum { OFF, INPUT, OUTPUT } MODE;
static struct {
	SOUNDBLASTER_CONFIG config;
//	int channel;

	MODE mode;
	int on;
	int dma;
	int frequency;
	int count;

	int input_state;
	int output_state;

	void *timer;
} blaster={ {0} } ;

void soundblaster_config(const SOUNDBLASTER_CONFIG *config)
{
	blaster.config = *config;
}

void soundblaster_reset(void)
{
	if (blaster.timer)
		blaster.timer = NULL;
	blaster.on=0;
	blaster.mode=OFF;
	blaster.input_state=0;
	blaster.output_state=0;
	blaster.frequency=0;
}

#if 0
int soundblaster_start(void)
{
	channel = stream_init("PC speaker", 50, Machine->sample_rate, 0, pc_sh_update);
    return 0;
}

void soundblaster_stop(void) {}
void soundblaster_update(void) {}
#endif

 READ8_HANDLER( soundblaster_r )
{
	int data=0;
	switch (offset) {
	case 0xa: //data input
		switch (blaster.input_state) {
		case 0:
			data=0xaa;
			break;
		case 1: // version high
			data=blaster.config.version.major;
			blaster.input_state++;
			break;
		case 2: // version low
			data=blaster.config.version.minor;
			blaster.input_state=0;
			break;
		case 3: // speaker state
			data=blaster.on?0xff:0;
			blaster.input_state=0;
			break;
		}
		break;
	case 0xc: //status
		break;
	case 0xe: // busy
		data=0x80;
		break;
	}
	return data;
}

static int soundblaster_operation(int data)
{
	switch (data) {
	case 0x10: /*play*/ return 1;
	case 0x40: /* set samplerate */ return 1;

	case 0xd0: /* dma break */ break;
	case 0xd4: /* dma continue */ break;
	case 0xd1: blaster.on=1; break;
	case 0xd3: blaster.on=0; break;
	case 0xd8: /* read speaker */ blaster.input_state=3; break;
	case 0xe1: /* read version */ blaster.input_state=1; break;
	}
	return 0;
}
	
WRITE8_HANDLER( soundblaster_w )
{
	switch (offset) {
	case 6: 
		if (data!=0) { //reset
		} else { // reset off
		}
		break;
	case 0xc: //operation, data
		switch (blaster.output_state) {
		case 0:
			blaster.output_state=soundblaster_operation(data);
			break;
		case 1:
			blaster.frequency=(int)(1000000.0/(256-data));
			blaster.output_state=0;
			break;
		case 10:
			DAC_data_w(0, data);
			blaster.output_state=0;
			break;
		}
		break;
	}
}

#if 0
struct CustomSound_interface soundblaster_interface = {
	soundblaster_start,
	soundblaster_stop,
	soundblaster_update
};
#endif
