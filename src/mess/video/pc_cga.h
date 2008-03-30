#include "driver.h"
#include "includes/crtc6845.h"
#include "video/pc_video.h"

#define CGA_PALETTE_SETS 83	/* one for colour, one for mono,
				 * 81 for colour composite */

#define CGA_SCREEN_NAME	"cga_screen"
#define CGA_MC6845_NAME	"mc6845_cga"

extern const unsigned char cga_palette[CGA_PALETTE_SETS * 16][3];

MACHINE_DRIVER_EXTERN( pcvideo_cga );

/* Set the base address for the character generation */
void pcvideo_cga_set_character_base( UINT8 *character_base );

/* has a special 640x200 in 16 color mode, 4 banks at 0xb8000 */
MACHINE_DRIVER_EXTERN( pcvideo_pc1512 );


/* Used in machine/pc.c */
READ16_HANDLER( pc1512_16le_r );
WRITE16_HANDLER( pc1512_16le_w );
WRITE16_HANDLER( pc1512_videoram16le_w );


/* Dipswitch for font selection */
#define CGA_FONT		(readinputport(20)&3)

/* Dipswitch for monitor selection */
#define CGA_MONITOR     (readinputport(20)&0x1C)
#define CGA_MONITOR_RGB         0x00    /* Colour RGB */
#define CGA_MONITOR_MONO        0x04    /* Greyscale RGB */
#define CGA_MONITOR_COMPOSITE   0x08    /* Colour composite */
#define CGA_MONITOR_TELEVISION  0x0C    /* Television */
#define CGA_MONITOR_LCD         0x10    /* LCD, eg PPC512 */

/* Dipswitch for chipset selection */
#define CGA_CHIPSET     (readinputport(20)&0xE0)
#define CGA_CHIPSET_IBM         0x00    /* Original IBM CGA */
#define CGA_CHIPSET_PC1512      0x20    /* PC1512 CGA subset */
#define CGA_CHIPSET_PC200       0x40    /* PC200 in CGA mode */
#define CGA_CHIPSET_ATI         0x60    /* ATI (supports Plantronics) */
#define CGA_CHIPSET_PARADISE    0x80    /* Paradise (used in PC1640) */

