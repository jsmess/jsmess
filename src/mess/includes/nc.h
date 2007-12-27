#include "driver.h"
#include "osdepend.h"
#define NC_NUM_COLOURS 4

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

#define NC200_NUM_COLOURS 4

/*----------- defined in video/nc.c -----------*/

extern VIDEO_START( nc );
extern VIDEO_UPDATE( nc );
extern PALETTE_INIT( nc );

void nc200_video_set_backlight(int state);


/*----------- defined in drivers/nc.c -----------*/

void nc_set_card_present_state(int);

/*----------- defined in machine/nc.c -----------*/

DEVICE_INIT( nc_pcmcia_card );
DEVICE_LOAD( nc_pcmcia_card );
DEVICE_UNLOAD( nc_pcmcia_card );

DEVICE_LOAD( nc_serial );

enum
{
        NC_TYPE_1xx, /* nc100/nc150 */
        NC_TYPE_200  /* nc200 */
};

