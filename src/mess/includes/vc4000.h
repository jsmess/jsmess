#include "driver.h"
#include "sound/custom.h"

// define this to use digital inputs instead of the slow
// autocentering analog mame joys
#define ANALOG_HACK

extern INTERRUPT_GEN( vc4000_video_line );
extern  READ8_HANDLER(vc4000_vsync_r);

extern  READ8_HANDLER(vc4000_video_r);
extern WRITE8_HANDLER(vc4000_video_w);

extern VIDEO_START( vc4000 );
extern VIDEO_UPDATE( vc4000 );

extern struct CustomSound_interface vc4000_sound_interface;

void vc4000_soundport_w (int mode, int data);
