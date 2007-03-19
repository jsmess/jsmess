#ifndef PC_T1T_H
#define PC_T1T_H

MACHINE_DRIVER_EXTERN( pcvideo_t1000hx );
MACHINE_DRIVER_EXTERN( pcvideo_t1000sx );

void pc_t1t_timer(void);
 READ8_HANDLER ( pc_t1t_videoram_r );
WRITE8_HANDLER ( pc_T1T_w );
 READ8_HANDLER (	pc_T1T_r );

void pc_t1t_reset(void);

#endif /* PC_T1T_H */
