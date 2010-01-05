/* 
    rmnimbus.c
    Machine driver for the Research Machines Nimbus.
    
    Phill Harvey-Smith
    2009-11-29.
*/

#include "machine/z80sio.h"
#include "machine/wd17xx.h"

#define DEBUG_TEXT  0x01
#define DEBUG_DB    0x02
#define DEBUG_PIXEL 0x04

#define MAINCPU_TAG "maincpu"
#define IOCPU_TAG   "iocpu"

DRIVER_INIT(nimbus);
MACHINE_RESET(nimbus);
MACHINE_START(nimbus);

/* 80186 Internal */
READ16_HANDLER (i186_internal_port_r);
WRITE16_HANDLER (i186_internal_port_w);

/* external int priority masks */

#define EXTINT_CTRL_PRI_MASK    0x07
#define EXTINT_CTRL_MSK         0x08
#define EXTINT_CTRL_LTM         0x10
#define EXTINT_CTRL_CASCADE     0x20
#define EXTINT_CTRL_SFNM        0x40

/* DMA control register */

#define DEST_MIO                0x8000
#define DEST_DECREMENT          0x4000
#define DEST_INCREMENT          0x2000
#define DEST_NO_CHANGE          (DEST_DECREMENT | DEST_INCREMENT)
#define DEST_INCDEC_MASK        (DEST_DECREMENT | DEST_INCREMENT)
#define SRC_MIO                 0X1000
#define SRC_DECREMENT           0x0800
#define SRC_INCREMENT           0x0400
#define SRC_NO_CHANGE           (SRC_DECREMENT | SRC_INCREMENT)
#define SRC_INCDEC_MASK         (SRC_DECREMENT | SRC_INCREMENT)
#define TERMINATE_ON_ZERO       0x0200
#define INTERRUPT_ON_ZERO       0x0100
#define SYNC_MASK               0x00C0
#define SYNC_SOURCE             0x0040
#define SYNC_DEST               0x0080
#define CHANNEL_PRIORITY        0x0020
#define TIMER_DRQ               0x0010
#define CHG_NOCHG               0x0004
#define ST_STOP                 0x0002
#define BYTE_WORD               0x0001

/* Nimbus specific */
READ16_HANDLER (nimbus_io_r);
WRITE16_HANDLER (nimbus_io_w);

/* External int vectors for chained interupts */
#define EXTERNAL_INT_FDC        0x80
#define EXTERNAL_INT_PC8031     0x8c
#define EXTERNAL_INT_Z80SIO     0x9C
#define EXTERNAL_INT_MSM5205    0x84

/* Z80 SIO for keyboard */

#define Z80SIO_TAG		    "z80sio"
#define NIMBUS_KEYROWS      11
#define KEYBOARD_QUEUE_SIZE 32

extern const z80sio_interface sio_intf;

READ8_DEVICE_HANDLER( sio_r );
WRITE8_DEVICE_HANDLER( sio_w );

void sio_interrupt(const device_config *device, int state);
//WRITE8_DEVICE_HANDLER( sio_dtr_w );
WRITE8_DEVICE_HANDLER( sio_serial_transmit );
int sio_serial_receive( const device_config *device, int channel );

/* Floppy drive interface */

#define FDC_TAG                 "wd2793"

extern const wd17xx_interface nimbus_wd17xx_interface;

READ8_HANDLER( nimbus_fdc_r );
WRITE8_HANDLER( nimbus_fdc_w );

#define NO_DRIVE_SELECTED   0xFF

/* 8031/8051 Peripheral controler */

#define PC8031_RAMSIZE          0X004

READ8_HANDLER( pc8031_r );
WRITE8_HANDLER( pc8031_w );

READ8_HANDLER( pc8031_iou_r );
WRITE8_HANDLER( pc8031_iou_w );

READ8_HANDLER( pc8031_port_r );
WRITE8_HANDLER( pc8031_port_w );

#define ER59256_TAG             "er59256"

/* IO unit */

#define FDC_INT_ENABLE          0x01
#define MSM5205_INT_ENABLE      0x04
#define PC8031_INT_ENABLE       0x10

READ8_HANDLER( iou_r );
WRITE8_HANDLER( iou_w );


/* Video stuff */
READ16_HANDLER (nimbus_video_io_r);
WRITE16_HANDLER (nimbus_video_io_w);

VIDEO_START( nimbus );
VIDEO_EOF( nimbus );
VIDEO_UPDATE( nimbus );
VIDEO_RESET( nimbus );

#define SCREEN_WIDTH_PIXELS     640
#define SCREEN_HEIGHT_LINES     250
#define SCREEN_NO_COLOURS       16

#define RED                     0
#define GREEN                   1
#define BLUE                    2

extern const unsigned char nimbus_palette[SCREEN_NO_COLOURS][3];

/* Sound hardware */

#define AY8910_TAG              "ay8910"
#define MONO_TAG                "mono"

READ8_HANDLER( sound_ay8910_r );
WRITE8_HANDLER( sound_ay8910_w );

WRITE8_HANDLER( sound_ay8910_porta_w );
WRITE8_HANDLER( sound_ay8910_portb_w );


#define MSM5205_TAG             "msm5205"

void nimbus_msm5205_vck(const device_config *device);

#define LINEAR_ADDR(seg,ofs)    ((seg<<4)+ofs)

/* Nimbus sub-bios structures for debugging */

typedef struct 
{
    UINT16  ofs_brush;
    UINT16  seg_brush;
    UINT16  ofs_data;
    UINT16  seg_data;
    UINT16  count;
} t_area_params;

typedef struct 
{
    UINT16  ofs_font;
    UINT16  seg_font;
    UINT16  ofs_data;
    UINT16  seg_data;
    UINT16  x;
    UINT16  y;
    UINT16  length;
} t_plot_string_params;

typedef struct
{
    UINT16  style;
    UINT16  style_index;
    UINT16  colour1;
    UINT16  colour2;
    UINT16  transparency;
    UINT16  boundary_spec;
    UINT16  boundary_colour;
    UINT16  save_colour;
} t_nimbus_brush;

#define OUTPUT_SEGOFS(mess,seg,ofs)  logerror("%s=%04X:%04X [%08X]\n",mess,seg,ofs,((seg<<4)+ofs))
