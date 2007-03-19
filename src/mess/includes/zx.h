/* from machine/zx80.c */
DRIVER_INIT( zx );
MACHINE_RESET( zx80 );
MACHINE_RESET( zx81 );
MACHINE_RESET( pc8300 );
MACHINE_RESET( pow3000 );

READ8_HANDLER ( zx_io_r );
WRITE8_HANDLER ( zx_io_w );

READ8_HANDLER ( pow3000_io_r );

/* from vidhrdw/zx80.c */
int zx_ula_scanline(void);
VIDEO_START( zx );
VIDEO_EOF( zx );

/* from vidhrdw/zx.c */
void zx_ula_bkgnd(int color);
int zx_ula_r(int offs, int region);

extern mame_timer *ula_nmi;
extern int ula_irq_active;
extern int ula_nmi_active;
extern int ula_frame_vsync;
extern int ula_scanline_count;
extern int ula_scancode_count;

