/*****************************************************************************
 *
 * includes/dai.h
 *
 ****************************************************************************/

#ifndef DAI_H_
#define DAI_H_


#define DAI_DEBUG	1


/*----------- defined in machine/dai.c -----------*/

MACHINE_START( dai );
READ8_HANDLER( dai_io_discrete_devices_r );
WRITE8_HANDLER( dai_io_discrete_devices_w );
WRITE8_HANDLER( dai_stack_interrupt_circuit_w );
READ8_HANDLER( amd9511_r );
WRITE8_HANDLER( amd9511_w );
extern UINT8 dai_noise_volume;
extern UINT8 dai_osc_volume[3];


/*----------- defined in video/dai.c -----------*/

extern const unsigned char dai_palette[16*3];
extern const unsigned short dai_colortable[1][16];
VIDEO_START( dai );
VIDEO_UPDATE( dai );
PALETTE_INIT( dai );


/*----------- defined in audio/dai.c -----------*/

extern const struct CustomSound_interface dai_sound_interface;
extern void dai_sh_change_clock(double);


#endif /* DAI_H_ */
