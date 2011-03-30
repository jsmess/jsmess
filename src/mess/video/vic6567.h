/*****************************************************************************
 *
 * video/vic6567.h
 *
 ****************************************************************************/

#ifndef __VIC6567_H__
#define __VIC6567_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*vic2_lightpen_x_callback)(running_machine &machine);
typedef UINT8 (*vic2_lightpen_y_callback)(running_machine &machine);
typedef UINT8 (*vic2_lightpen_button_callback)(running_machine &machine);

typedef int (*vic2_dma_read)(running_machine &machine, int);
typedef int (*vic2_dma_read_color)(running_machine &machine, int);
typedef void (*vic2_irq) (running_machine&, int);

typedef UINT8 (*vic2_rdy_callback)(running_machine &machine);

typedef enum
{
	VIC6567,	// VIC II NTSC
	VIC6569,	// VIC II PAL
      VIC8564,	// VIC IIe NTSC
      VIC8566	// VIC IIe PAL
} vic2_type;

typedef struct _vic2_interface vic2_interface;
struct _vic2_interface
{
	const char         *screen;
	const char         *cpu;

	vic2_type type;

	vic2_lightpen_x_callback        x_cb;
	vic2_lightpen_y_callback        y_cb;
	vic2_lightpen_button_callback   button_cb;

	vic2_dma_read          dma_read;
	vic2_dma_read_color    dma_read_color;
	vic2_irq               irq;

	vic2_rdy_callback        rdy_cb;
};

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define VIC6567_CLOCK		(1022700 /* = 8181600 / 8) */ )
#define VIC6569_CLOCK		( 985248 /* = 7881984 / 8) */ )

#define VIC6567_CYCLESPERLINE	65
#define VIC6569_CYCLESPERLINE	63

#define VIC6567_LINES		263
#define VIC6569_LINES		312

#define VIC6567_VRETRACERATE	(59.8245100906698 /* = 1022700 / (65 * 263) */ )
#define VIC6569_VRETRACERATE	(50.1245421245421 /* =  985248 / (63 * 312) */ )

#define VIC6567_HRETRACERATE	(VIC6567_CLOCK / VIC6567_CYCLESPERLINE)
#define VIC6569_HRETRACERATE	(VIC6569_CLOCK / VIC6569_CYCLESPERLINE)

#define VIC2_HSIZE		320
#define VIC2_VSIZE		200

#define VIC6567_VISIBLELINES	235
#define VIC6569_VISIBLELINES	284

#define VIC6567_FIRST_DMA_LINE	0x30
#define VIC6569_FIRST_DMA_LINE	0x30

#define VIC6567_LAST_DMA_LINE	0xf7
#define VIC6569_LAST_DMA_LINE	0xf7

#define VIC6567_FIRST_DISP_LINE	0x29
#define VIC6569_FIRST_DISP_LINE	0x10

#define VIC6567_LAST_DISP_LINE	(VIC6567_FIRST_DISP_LINE + VIC6567_VISIBLELINES - 1)
#define VIC6569_LAST_DISP_LINE	(VIC6569_FIRST_DISP_LINE + VIC6569_VISIBLELINES - 1)

#define VIC6567_RASTER_2_EMU(a) ((a >= VIC6567_FIRST_DISP_LINE) ? (a - VIC6567_FIRST_DISP_LINE) : (a + 222))
#define VIC6569_RASTER_2_EMU(a) (a - VIC6569_FIRST_DISP_LINE)

#define VIC6567_FIRSTCOLUMN	50
#define VIC6569_FIRSTCOLUMN	50

#define VIC6567_VISIBLECOLUMNS	418
#define VIC6569_VISIBLECOLUMNS	403

#define VIC6567_X_2_EMU(a)	(a)
#define VIC6569_X_2_EMU(a)	(a)

#define VIC6567_STARTVISIBLELINES ((VIC6567_LINES - VIC6567_VISIBLELINES)/2)
#define VIC6569_STARTVISIBLELINES 16 /* ((VIC6569_LINES - VIC6569_VISIBLELINES)/2) */

#define VIC6567_FIRSTRASTERLINE 34
#define VIC6569_FIRSTRASTERLINE 0

#define VIC6567_COLUMNS 512
#define VIC6569_COLUMNS 504


#define VIC6567_STARTVISIBLECOLUMNS ((VIC6567_COLUMNS - VIC6567_VISIBLECOLUMNS)/2)
#define VIC6569_STARTVISIBLECOLUMNS ((VIC6569_COLUMNS - VIC6569_VISIBLECOLUMNS)/2)

#define VIC6567_FIRSTRASTERCOLUMNS 412
#define VIC6569_FIRSTRASTERCOLUMNS 404

#define VIC6569_FIRST_X 0x194
#define VIC6567_FIRST_X 0x19c

#define VIC6569_FIRST_VISIBLE_X 0x1e0
#define VIC6567_FIRST_VISIBLE_X 0x1e8

#define VIC6569_MAX_X 0x1f7
#define VIC6567_MAX_X 0x1ff

#define VIC6569_LAST_VISIBLE_X 0x17c
#define VIC6567_LAST_VISIBLE_X 0x184

#define VIC6569_LAST_X 0x193
#define VIC6567_LAST_X 0x19b


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(VIC2, vic2);

#define MCFG_VIC2_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, VIC2, 0) \
	MCFG_DEVICE_CONFIG(_interface)


/*----------- defined in video/vic6567.c -----------*/

WRITE8_DEVICE_HANDLER ( vic2_port_w );
READ8_DEVICE_HANDLER ( vic2_port_r );

int vic2e_k0_r(device_t *device);
int vic2e_k1_r(device_t *device);
int vic2e_k2_r(device_t *device);

void vic2_raster_interrupt_gen( device_t *device );
UINT32 vic2_video_update( device_t *device, bitmap_t *bitmap, const rectangle *cliprect );


extern void vic2_set_rastering(device_t *device, int onoff);
extern void vic2_lightpen_write(device_t *device, int level);



#endif /* __VIC6567_H__ */
