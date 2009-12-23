/*****************************************************************************
 *
 * includes/nc.h
 *
 ****************************************************************************/

#ifndef NC_H_
#define NC_H_


#define NC_NUM_COLOURS 4

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

#define NC200_NUM_COLOURS 4

enum
{
	NC_TYPE_1xx, /* nc100/nc150 */
	NC_TYPE_200  /* nc200 */
};


/*----------- defined in video/nc.c -----------*/

extern VIDEO_START( nc );
extern VIDEO_UPDATE( nc );
extern PALETTE_INIT( nc );

void nc200_video_set_backlight(int state);


/*----------- defined in drivers/nc.c -----------*/

/* pointer to loaded data */
extern unsigned char *nc_card_ram;
/* mask used to stop access over end of card ram area */
extern int nc_membank_card_ram_mask;

extern unsigned long nc_display_memory_start;
extern UINT8 nc_type;

void nc_set_card_present_state(int);


/*----------- defined in machine/nc.c -----------*/

DEVICE_START( nc_pcmcia_card );
DEVICE_IMAGE_LOAD( nc_pcmcia_card );
DEVICE_IMAGE_UNLOAD( nc_pcmcia_card );

#endif /* NC_H_ */
