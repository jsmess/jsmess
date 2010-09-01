#ifndef _MACHINE_CDIC_H_
#define _MACHINE_CDIC_H_

#include "devices/chd_cd.h"

namespace CDI
{

namespace Mono1
{

#define CDIC_BUFFERED_SECTORS	2

class CDIC
{

public:
    CDIC();
    ~CDIC();

private:
	UINT16 command; 			// CDIC Command Register (0x303c00)
	UINT32 time;				// CDIC Time Register (0x303c02)
	UINT16 file;				// CDIC File Register (0x303c06)
	UINT32 channel; 			// CDIC Channel Register (0x303c08)
	UINT16 audio_channel;		// CDIC Audio Channel Register (0x303c0c)

	UINT16 audio_buffer;		// CDIC Audio Buffer Register (0x303ff4)
	UINT16 x_buffer;			// CDIC X-Buffer Register (0x303ff6)
	UINT16 dma_control;			// CDIC DMA Control Register (0x303ff8)
	UINT16 z_buffer;			// CDIC Z-Buffer Register (0x303ffa)
	UINT16 interrupt_vector;	// CDIC Interrupt Vector Register (0x303ffc)
	UINT16 data_buffer;			// CDIC Data Buffer Register (0x303ffe)

	emu_timer *interrupt_timer;
	cdrom_file *cd;

	emu_timer *audio_sample_timer;
	INT32 audio_sample_freq;
	INT32 audio_sample_size;

	UINT16 decode_addr;
	UINT8 decode_delay;
	attotime decode_period;

	int xa_last[4];
	UINT16 *ram;
};

const INT32 CDIC_SECTOR_SYNC        0

const INT32 CDIC_SECTOR_HEADER      12

const INT32 CDIC_SECTOR_MODE        15

const INT32 CDIC_SECTOR_FILE1       16
const INT32 CDIC_SECTOR_CHAN1       17
const INT32 CDIC_SECTOR_SUBMODE1    18
const INT32 CDIC_SECTOR_CODING1     19

const INT32 CDIC_SECTOR_FILE2       20
const INT32 CDIC_SECTOR_CHAN2       21
const INT32 CDIC_SECTOR_SUBMODE2    22
const INT32 CDIC_SECTOR_CODING2     23

const INT32 CDIC_SECTOR_DATA        24

const INT32 CDIC_SECTOR_SIZE        2352

const INT32 CDIC_SECTOR_DATASIZE    2048
const INT32 CDIC_SECTOR_AUDIOSIZE   2304
const INT32 CDIC_SECTOR_VIDEOSIZE   2324

const INT32 CDIC_SUBMODE_EOF        0x80
const INT32 CDIC_SUBMODE_RT         0x40
const INT32 CDIC_SUBMODE_FORM       0x20
const INT32 CDIC_SUBMODE_TRIG       0x10
const INT32 CDIC_SUBMODE_DATA       0x08
const INT32 CDIC_SUBMODE_AUDIO      0x04
const INT32 CDIC_SUBMODE_VIDEO      0x02
const INT32 CDIC_SUBMODE_EOR        0x01

}

}

#endif // _MACHINE_CDIC_H_
