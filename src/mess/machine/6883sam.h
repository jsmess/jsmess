/**********************************************************************

	Motorola 6883 SAM interface and emulation

	This function emulates all the functionality of one M6883
	synchronous address multiplexer.

	Note that the real SAM chip was intimately involved in things like
	memory and video addressing, which are things that the MAME core
	largely handles.  Thus, this code only takes care of a small part
	of the SAM's actual functionality; it simply tracks the SAM
	registers and handles things like save states.  It then delegates
	the bulk of the responsibilities back to the host.

**********************************************************************/

#ifndef __6833SAM_H__
#define __6833SAM_H__

#include "driver.h"

typedef enum
{
	SAM6883_ORIGINAL,
	SAM6883_GIME
} sam6883_type;

typedef struct _sam6883_interface sam6883_interface;
struct _sam6883_interface
{
	sam6883_type type;
	const UINT8 *(*get_rambase)(void);
	void (*set_pageonemode)(int val);
	void (*set_mpurate)(int val);
	void (*set_memorysize)(int val);
	void (*set_maptype)(int val);
};

/* initialize the SAM */
void sam_init(running_machine *machine, const sam6883_interface *intf);

/* set the state of the SAM */
void sam_set_state(UINT16 state, UINT16 mask);

/* used by vidhrdw/m6847.c to read the position of the SAM */
const UINT8 *sam_m6847_get_video_ram(int scanline);

/* write to the SAM */
WRITE8_HANDLER(sam_w);

/* used to get memory size and pagemode independent of callbacks */
UINT8 get_sam_memorysize(void);
UINT8 get_sam_pagemode(void);
UINT8 get_sam_maptype(void);

#endif /* __6833SAM_H__ */
