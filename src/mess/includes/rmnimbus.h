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
#define EXTERNAL_INT_DISK       0x80
#define EXTERNAL_INT_PC8031_8C  0x8c
#define EXTERNAL_INT_PC8031_8E  0x8E
#define EXTERNAL_INT_PC8031_8F  0x8F
#define EXTERNAL_INT_Z80SIO     0x9C
#define EXTERNAL_INT_MSM5205    0x84

/* Z80 SIO for keyboard */

#define Z80SIO_TAG		    "z80sio"
#define NIMBUS_KEYROWS      11
#define KEYBOARD_QUEUE_SIZE 32

extern const z80sio_interface sio_intf;

READ8_DEVICE_HANDLER( sio_r );
WRITE8_DEVICE_HANDLER( sio_w );

void sio_interrupt(running_device *device, int state);
//WRITE8_DEVICE_HANDLER( sio_dtr_w );
WRITE8_DEVICE_HANDLER( sio_serial_transmit );
int sio_serial_receive( running_device *device, int channel );

/* Floppy/Fixed drive interface */

#define FDC_TAG                 "wd2793"

extern const wd17xx_interface nimbus_wd17xx_interface;

READ8_HANDLER( nimbus_disk_r );
WRITE8_HANDLER( nimbus_disk_w );

#define NO_DRIVE_SELECTED   0xFF

/* SASI harddisk interface */
#define HARDDISK0_TAG           "harddisk0"
#define SCSIBUS_TAG             "scsibus"

void nimbus_scsi_linechange(running_device *device, UINT8 line, UINT8 state); 

/* Masks for writes to port 0x400 */
#define FDC_DRIVE0_MASK 0x01
#define FDC_DRIVE1_MASK 0x02
#define FDC_DRIVE2_MASK 0x04
#define FDC_DRIVE3_MASK 0x08
#define FDC_SIDE_MASK   0x10
#define FDC_MOTOR_MASKO 0x20
#define HDC_DRQ_MASK    0x40
#define FDC_DRQ_MASK    0x80
#define FDC_DRIVE_MASK  (FDC_DRIVE0_MASK | FDC_DRIVE1_MASK | FDC_DRIVE2_MASK | FDC_DRIVE3_MASK)

#define FDC_SIDE()          ((nimbus_drives.reg400 & FDC_SIDE_MASK) >> 4)
#define FDC_MOTOR()         ((nimbus_drives.reg400 & FDC_MOTOR_MASKO) >> 5)
#define FDC_DRIVE()         (driveno(nimbus_drives.reg400 & FDC_DRIVE_MASK))
#define HDC_DRQ_ENABLED()   ((nimbus_drives.reg400 & HDC_DRQ_MASK) ? 1 : 0)
#define FDC_DRQ_ENABLED()   ((nimbus_drives.reg400 & FDC_DRQ_MASK) ? 1 : 0)

/* Masks for port 0x410 read*/

#define FDC_READY_MASK  0x01
#define FDC_INDEX_MASK  0x02
#define FDC_MOTOR_MASKI 0x04
#define HDC_MSG_MASK    0x08
#define HDC_BSY_MASK    0x10
#define HDC_IO_MASK     0X20
#define HDC_CD_MASK     0x40
#define HDC_REQ_MASK    0x80

#define FDC_BITS_410    (FDC_READY_MASK | FDC_INDEX_MASK | FDC_MOTOR_MASKI)
#define HDC_BITS_410    (HDC_MSG_MASK | HDC_BSY_MASK | HDC_IO_MASK | HDC_CD_MASK | HDC_REQ_MASK)
#define INV_BITS_410    (HDC_BSY_MASK | HDC_IO_MASK | HDC_CD_MASK | HDC_REQ_MASK)

#define HDC_INT_TRIGGER (HDC_IO_MASK | HDC_CD_MASK | HDC_REQ_MASK)

/* Masks for port 0x410 write*/

#define HDC_RESET_MASK  0x01
#define HDC_SEL_MASK    0x02
#define HDC_IRQ_MASK    0x04
#define HDC_IRQ_ENABLED()   ((nimbus_drives.reg410_out & HDC_IRQ_MASK) ? 1 : 0)


#define SCSI_ID_NONE    0x80


/* 8031/8051 Peripheral controler */

#define IPC_OUT_ADDR        0X01
#define IPC_OUT_READ_PEND   0X02
#define IPC_OUT_BYTE_AVAIL  0X04

#define IPC_IN_ADDR         0X01
#define IPC_IN_BYTE_AVAIL   0X02
#define IPC_IN_READ_PEND    0X04

READ8_HANDLER( pc8031_r );
WRITE8_HANDLER( pc8031_w );

READ8_HANDLER( pc8031_iou_r );
WRITE8_HANDLER( pc8031_iou_w );

READ8_HANDLER( pc8031_port_r );
WRITE8_HANDLER( pc8031_port_w );

#define ER59256_TAG             "er59256"

/* IO unit */

#define DISK_INT_ENABLE         0x01
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

void nimbus_msm5205_vck(running_device *device);

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
