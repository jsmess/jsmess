/*----------- defined in drivers/starshp1.c -----------*/

extern int starshp1_attract;


/*----------- defined in video/starshp1.c -----------*/

extern unsigned char *starshp1_playfield_ram;
extern unsigned char *starshp1_hpos_ram;
extern unsigned char *starshp1_vpos_ram;
extern unsigned char *starshp1_obj_ram;

extern int starshp1_ship_explode;
extern int starshp1_ship_picture;
extern int starshp1_ship_hoffset;
extern int starshp1_ship_voffset;
extern int starshp1_ship_size;

extern int starshp1_circle_hpos;
extern int starshp1_circle_vpos;
extern int starshp1_circle_size;
extern int starshp1_circle_mod;
extern int starshp1_circle_kill;

extern int starshp1_phasor;
extern int starshp1_collision_latch;
extern int starshp1_starfield_kill;
extern int starshp1_mux;

READ8_HANDLER( starshp1_rng_r );

WRITE8_HANDLER( starshp1_sspic_w );
WRITE8_HANDLER( starshp1_ssadd_w );
WRITE8_HANDLER( starshp1_playfield_w );

VIDEO_UPDATE( starshp1 );
VIDEO_EOF( starshp1 );
VIDEO_START( starshp1 );
