#include "driver.h"
#include "sound/custom.h"

extern INTERRUPT_GEN( arcadia_video_line );
 READ8_HANDLER(arcadia_vsync_r);

 READ8_HANDLER(arcadia_video_r);
WRITE8_HANDLER(arcadia_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 0
//#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 32

extern VIDEO_START( arcadia );
extern VIDEO_UPDATE( arcadia );

extern struct CustomSound_interface arcadia_sound_interface;
extern void arcadia_soundport_w (int mode, int data);
