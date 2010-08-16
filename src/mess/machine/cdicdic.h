/******************************************************************************


    CD-i Mono-I CDIC MCU simulation
    -------------------

    MESS implementation by Harmony


*******************************************************************************

STATUS:

- Just enough for the Mono-I CD-i board to work somewhat properly.

TODO:

- Decapping and proper emulation.

*******************************************************************************/

#ifndef _MACHINE_CDICDIC_H_
#define _MACHINE_CDICDIC_H_

#include "emu.h"
#include "devices/chd_cd.h"

#define CDIC_BUFFERED_SECTORS   2

typedef struct
{
    UINT16 command;             // CDIC Command Register (0x303c00)
    UINT32 time;                // CDIC Time Register (0x303c02)
    UINT16 file;                // CDIC File Register (0x303c06)
    UINT32 channel;             // CDIC Channel Register (0x303c08)
    UINT16 audio_channel;       // CDIC Audio Channel Register (0x303c0c)

    UINT16 audio_buffer;        // CDIC Audio Buffer Register (0x303ff4)
    UINT16 x_buffer;            // CDIC X-Buffer Register (0x303ff6)
    UINT16 dma_control;         // CDIC DMA Control Register (0x303ff8)
    UINT16 z_buffer;            // CDIC Z-Buffer Register (0x303ffa)
    UINT16 interrupt_vector;    // CDIC Interrupt Vector Register (0x303ffc)
    UINT16 data_buffer;         // CDIC Data Buffer Register (0x303ffe)

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
} cdic_regs_t;

#define CDIC_SECTOR_SYNC        0

#define CDIC_SECTOR_HEADER      12

#define CDIC_SECTOR_MODE        15

#define CDIC_SECTOR_FILE1       16
#define CDIC_SECTOR_CHAN1       17
#define CDIC_SECTOR_SUBMODE1    18
#define CDIC_SECTOR_CODING1     19

#define CDIC_SECTOR_FILE2       20
#define CDIC_SECTOR_CHAN2       21
#define CDIC_SECTOR_SUBMODE2    22
#define CDIC_SECTOR_CODING2     23

#define CDIC_SECTOR_DATA        24

#define CDIC_SECTOR_SIZE        2352

#define CDIC_SECTOR_DATASIZE    2048
#define CDIC_SECTOR_AUDIOSIZE   2304
#define CDIC_SECTOR_VIDEOSIZE   2324

#define CDIC_SUBMODE_EOF        0x80
#define CDIC_SUBMODE_RT         0x40
#define CDIC_SUBMODE_FORM       0x20
#define CDIC_SUBMODE_TRIG       0x10
#define CDIC_SUBMODE_DATA       0x08
#define CDIC_SUBMODE_AUDIO      0x04
#define CDIC_SUBMODE_VIDEO      0x02
#define CDIC_SUBMODE_EOR        0x01

// Member functions
extern TIMER_CALLBACK( audio_sample_trigger );
extern TIMER_CALLBACK( cdic_trigger_readback_int );
extern READ16_HANDLER( cdic_r );
extern WRITE16_HANDLER( cdic_w );
extern void cdic_init(running_machine *machine, cdic_regs_t *cdic);
extern void cdic_register_globals(running_machine *machine, cdic_regs_t *cdic);

#endif // _MACHINE_CDICDIC_H_
