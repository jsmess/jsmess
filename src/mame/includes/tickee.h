/*************************************************************************

    Raster Elite Tickee Tickats hardware

**************************************************************************/

/*----------- defined in drivers/tickee.c -----------*/

extern UINT16 *tickee_control;


/*----------- defined in video/tickee.c -----------*/

extern UINT16 *tickee_vram;

VIDEO_START( tickee );

VIDEO_UPDATE( tickee );
