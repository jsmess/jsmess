/* machine/p2000t.c */
extern  READ8_HANDLER( p2000t_port_000f_r );
extern  READ8_HANDLER( p2000t_port_202f_r );
extern WRITE8_HANDLER( p2000t_port_000f_w );
extern WRITE8_HANDLER( p2000t_port_101f_w );
extern WRITE8_HANDLER( p2000t_port_303f_w );
extern WRITE8_HANDLER( p2000t_port_505f_w );
extern WRITE8_HANDLER( p2000t_port_707f_w );
extern WRITE8_HANDLER( p2000t_port_888b_w );
extern WRITE8_HANDLER( p2000t_port_8c90_w );
extern WRITE8_HANDLER( p2000t_port_9494_w );

/* vidhrdw/p2000t.c */
extern void p2000m_vh_callback (void);
extern VIDEO_START( p2000m );
extern VIDEO_UPDATE( p2000m );

/* systems/p2000t.c */

