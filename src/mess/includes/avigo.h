#include "driver.h"
#include "osdepend.h"
#define AVIGO_NUM_COLOURS 2

#define AVIGO_SCREEN_WIDTH        160
#define AVIGO_SCREEN_HEIGHT       240

extern  READ8_HANDLER(avigo_vid_memory_r);
extern WRITE8_HANDLER(avigo_vid_memory_w);

extern VIDEO_START( avigo );
extern VIDEO_UPDATE( avigo );
extern PALETTE_INIT( avigo );

void	avigo_vh_set_stylus_marker_position(int x,int y);

