#include "driver.h"
#include "includes/crtc6845.h"
#include "video/pc_video.h"

#define CGA_PALETTE_SETS 83	/* one for colour, one for mono,
				 * 81 for colour composite */

extern unsigned char cga_palette[CGA_PALETTE_SETS * 16][3];
extern unsigned short cga_colortable[256*2 + 16*2 + 96*4];
extern gfx_layout CGA_charlayout;
extern gfx_layout CGA_gfxlayout_1bpp;
extern gfx_layout CGA_gfxlayout_2bpp;

MACHINE_DRIVER_EXTERN( pcvideo_cga );

/* has a special 640x200 in 16 color mode, 4 banks at 0xb8000 */
MACHINE_DRIVER_EXTERN( pcvideo_pc1512 );

pc_video_update_proc pc_cga_choosevideomode(int *width, int *height, struct crtc6845 *crtc);

READ8_HANDLER( pc_cga8_r );
WRITE8_HANDLER( pc_cga8_w );
READ32_HANDLER( pc_cga32_r );
WRITE32_HANDLER( pc_cga32_w );

VIDEO_START( pc1512 );

 READ8_HANDLER ( pc1512_r );
WRITE8_HANDLER ( pc1512_w );
WRITE8_HANDLER ( pc1512_videoram_w );

#define CGA_FONT		(input_port_20_r(0)&3)

//internal use
void pc_cga_cursor(struct crtc6845_cursor *cursor);



