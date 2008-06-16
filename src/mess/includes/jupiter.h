/*****************************************************************************
 *
 * includes/jupiter.h
 *
 ****************************************************************************/

#ifndef JUPITER_H_
#define JUPITER_H_


/*----------- defined in machine/jupiter.c -----------*/

OPBASE_HANDLER( jupiter_opbaseoverride );
MACHINE_START( jupiter );

DEVICE_IMAGE_LOAD( jupiter_ace );
DEVICE_IMAGE_LOAD( jupiter_tap );
DEVICE_IMAGE_UNLOAD( jupiter_tap );

READ8_HANDLER( jupiter_port_7ffe_r);
WRITE8_HANDLER( jupiter_port_fe_w);


/*----------- defined in video/jupiter.c -----------*/

VIDEO_UPDATE( jupiter );
WRITE8_HANDLER( jupiter_vh_charram_w );
extern unsigned char *jupiter_charram;
extern size_t jupiter_charram_size;


#endif /* JUPITER_H_ */
