#ifndef DAI_H
#define DAI_H

#define DAI_DEBUG	1

/* machine/dai.c */
MACHINE_START( dai );
READ8_HANDLER( dai_io_discrete_devices_r );
WRITE8_HANDLER( dai_io_discrete_devices_w );
WRITE8_HANDLER( dai_stack_interrupt_circuit_w );
READ8_HANDLER( amd9511_r );
WRITE8_HANDLER( amd9511_w );
extern UINT8 dai_noise_volume;
extern UINT8 dai_osc_volume[3];

/* vidhrdw/dai.c */
extern unsigned char dai_palette[16*3];
extern unsigned short dai_colortable[1][16];
VIDEO_START( dai );
VIDEO_UPDATE( dai );
PALETTE_INIT( dai );
void dai_update_palette (UINT8);

/* sndhrdw/dai.c */
extern struct CustomSound_interface dai_sound_interface;
extern void dai_sh_change_clock(double);

#endif /* DAI_H */
