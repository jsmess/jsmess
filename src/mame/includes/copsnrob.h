/*************************************************************************

    Atari Cops'n Robbers hardware

*************************************************************************/

/*----------- defined in machine/copsnrob.c -----------*/

READ8_HANDLER( copsnrob_gun_position_r );


/*----------- defined in video/copsnrob.c -----------*/

extern unsigned char *copsnrob_bulletsram;
extern unsigned char *copsnrob_carimage;
extern unsigned char *copsnrob_cary;
extern unsigned char *copsnrob_trucky;
extern unsigned char *copsnrob_truckram;

VIDEO_UPDATE( copsnrob );
