#ifndef AQUARIUS_H
#define AQUARIUS_H

/* machine/aquarius.c */
extern MACHINE_RESET( aquarius );
extern  READ8_HANDLER( aquarius_port_ff_r );
extern  READ8_HANDLER( aquarius_port_fe_r );
extern WRITE8_HANDLER( aquarius_port_fc_w );
extern WRITE8_HANDLER( aquarius_port_fe_w );
extern WRITE8_HANDLER( aquarius_port_ff_w );

/* vidhrdw/aquarius.c */
extern VIDEO_START( aquarius );
extern VIDEO_UPDATE( aquarius );
extern WRITE8_HANDLER( aquarius_videoram_w );

#endif /* AQUARIUS_H */
