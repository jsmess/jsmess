/*****************************************************************************
 *
 * includes/pocketc.h
 *
 ****************************************************************************/

#ifndef POCKETC_H_
#define POCKETC_H_


class pocketc_state : public driver_device
{
public:
	pocketc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in video/pocketc.c -----------*/

extern PALETTE_INIT( pocketc );
extern VIDEO_START( pocketc );

extern const unsigned short pocketc_colortable[8][2];

typedef const char *POCKETC_FIGURE[];
void pocketc_draw_special(bitmap_t *bitmap,
						  int x, int y, const POCKETC_FIGURE fig, int color);


#endif /* POCKETC_H_ */
