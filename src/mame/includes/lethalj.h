/*************************************************************************

    The Game Room Lethal Justice hardware

**************************************************************************/

/*----------- defined in video/lethalj.c -----------*/

READ16_HANDLER( lethalj_gun_r );

VIDEO_START( lethalj );

WRITE16_HANDLER( lethalj_blitter_w );

VIDEO_UPDATE( lethalj );
VIDEO_UPDATE( laigames );

