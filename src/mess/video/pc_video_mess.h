/*********************************************************************

    pc_video.h

    Refactoring of code common to PC video implementations

*********************************************************************/

#ifndef PC_VIDEO_H
#define PC_VIDEO_H

#include "includes/crtc6845.h"

extern size_t pc_videoram_size;

typedef void (*pc_video_update_proc)(bitmap_t *bitmap,
	struct mscrtc6845 *crtc);

struct mscrtc6845 *pc_video_start(running_machine *machine, const struct mscrtc6845_config *config,
	pc_video_update_proc (*choosevideomode)(running_machine *machine, int *width, int *height, struct mscrtc6845 *crtc),
	size_t vramsize);

VIDEO_UPDATE( pc_video );

WRITE8_HANDLER( pc_video_videoram_w );
WRITE16_HANDLER( pc_video_videoram16le_w );
WRITE32_HANDLER( pc_video_videoram32_w );

#endif /* PC_VIDEO_H */
