/***************************************************************************

    Midway 8080-based black and white hardware

****************************************************************************/


#define MW8080BW_MASTER_CLOCK				(19968000)
#define MW8080BW_CPU_CLOCK					(MW8080BW_MASTER_CLOCK / 10)
#define MW8080BW_PIXEL_CLOCK				(MW8080BW_MASTER_CLOCK / 4)
#define MW8080BW_HTOTAL						(0x140)
#define MW8080BW_HBEND						(0x000)
#define MW8080BW_HBSTART					(0x100)
#define MW8080BW_VTOTAL						(0x106)
#define MW8080BW_VBEND						(0x000)
#define MW8080BW_VBSTART					(0x0e0)
#define MW8080BW_VCOUNTER_START_NO_VBLANK	(0x020)
#define MW8080BW_VCOUNTER_START_VBLANK		(0x0da)
#define MW8080BW_INT_TRIGGER_COUNT_1		(0x080)
#define MW8080BW_INT_TRIGGER_VBLANK_1		(0)
#define MW8080BW_INT_TRIGGER_COUNT_2		MW8080BW_VCOUNTER_START_VBLANK
#define MW8080BW_INT_TRIGGER_VBLANK_2		(1)

/* +4 is added to HBSTART because the hardware displays that many pixels after
   setting HBLANK */
#define MW8080BW_HPIXCOUNT		(MW8080BW_HBSTART + 4)


/*----------- defined in drivers/mw8080bw.c -----------*/

UINT8 mw8080bw_ram_r(offs_t offset);
MACHINE_DRIVER_EXTERN( mw8080bw_root );
MACHINE_DRIVER_EXTERN( invaders );
extern const char layout_invaders[];

#define SEAWOLF_GUN_PORT_TAG			("GUN")

#define TORNBASE_CAB_TYPE_UPRIGHT_OLD	(0)
#define TORNBASE_CAB_TYPE_UPRIGHT_NEW	(1)
#define TORNBASE_CAB_TYPE_COCKTAIL		(2)
UINT8 tornbase_get_cabinet_type(void);

#define DESERTGU_GUN_X_PORT_TAG			("GUNX")
#define DESERTGU_GUN_Y_PORT_TAG			("GUNY")
void desertgun_set_controller_select(UINT8 data);

void clowns_set_controller_select(UINT8 data);

void spcenctr_set_strobe_state(UINT8 data);
UINT8 spcenctr_get_trench_width(void);
UINT8 spcenctr_get_trench_center(void);
UINT8 spcenctr_get_trench_slope(UINT8 addr);

UINT16 phantom2_get_cloud_counter(void);
void phantom2_set_cloud_counter(UINT16 data);

UINT8 invaders_is_flip_screen(void);
void invaders_set_flip_screen(UINT8 data);
int invaders_is_cabinet_cocktail(void);

#define BLUESHRK_SPEAR_PORT_TAG			("SPEAR")


/*----------- defined in machine/mw8080bw.c -----------*/

MACHINE_START( mw8080bw );
MACHINE_RESET( mw8080bw );


/*----------- defined in audio/mw8080bw.c -----------*/

MACHINE_START( mw8080bw_audio );

WRITE8_HANDLER( midway_tone_generator_lo_w );
WRITE8_HANDLER( midway_tone_generator_hi_w );

MACHINE_DRIVER_EXTERN( seawolf_sound );
WRITE8_HANDLER( seawolf_sh_port_w );

MACHINE_DRIVER_EXTERN( gunfight_sound );
WRITE8_HANDLER( gunfight_sh_port_w );

MACHINE_DRIVER_EXTERN( tornbase_sound );
WRITE8_HANDLER( tornbase_sh_port_w );

MACHINE_DRIVER_EXTERN( zzzap_sound );
WRITE8_HANDLER( zzzap_sh_port_1_w );
WRITE8_HANDLER( zzzap_sh_port_2_w );

MACHINE_DRIVER_EXTERN( maze_sound );
void maze_write_discrete(UINT8 maze_tone_timing_state);

MACHINE_DRIVER_EXTERN( boothill_sound );
WRITE8_HANDLER( boothill_sh_port_w );

MACHINE_DRIVER_EXTERN( checkmat_sound );
WRITE8_HANDLER( checkmat_sh_port_w );

MACHINE_DRIVER_EXTERN( desertgu_sound );
WRITE8_HANDLER( desertgu_sh_port_1_w );
WRITE8_HANDLER( desertgu_sh_port_2_w );

MACHINE_DRIVER_EXTERN( dplay_sound );
WRITE8_HANDLER( dplay_sh_port_w );

MACHINE_DRIVER_EXTERN( gmissile_sound );
WRITE8_HANDLER( gmissile_sh_port_1_w );
WRITE8_HANDLER( gmissile_sh_port_2_w );
WRITE8_HANDLER( gmissile_sh_port_3_w );

MACHINE_DRIVER_EXTERN( m4_sound );
WRITE8_HANDLER( m4_sh_port_1_w );
WRITE8_HANDLER( m4_sh_port_2_w );

MACHINE_DRIVER_EXTERN( clowns_sound );
WRITE8_HANDLER( clowns_sh_port_1_w );
WRITE8_HANDLER( clowns_sh_port_2_w );

MACHINE_DRIVER_EXTERN( shuffle_sound );
WRITE8_HANDLER( shuffle_sh_port_1_w );
WRITE8_HANDLER( shuffle_sh_port_2_w );

MACHINE_DRIVER_EXTERN( dogpatch_sound );
WRITE8_HANDLER( dogpatch_sh_port_w );

MACHINE_DRIVER_EXTERN( spcenctr_sound );
WRITE8_HANDLER( spcenctr_sh_port_1_w );
WRITE8_HANDLER( spcenctr_sh_port_2_w );
WRITE8_HANDLER( spcenctr_sh_port_3_w );

MACHINE_DRIVER_EXTERN( phantom2_sound );
WRITE8_HANDLER( phantom2_sh_port_1_w );
WRITE8_HANDLER( phantom2_sh_port_2_w );

MACHINE_DRIVER_EXTERN( bowler_sound );
WRITE8_HANDLER( bowler_sh_port_1_w );
WRITE8_HANDLER( bowler_sh_port_2_w );
WRITE8_HANDLER( bowler_sh_port_3_w );
WRITE8_HANDLER( bowler_sh_port_4_w );
WRITE8_HANDLER( bowler_sh_port_5_w );
WRITE8_HANDLER( bowler_sh_port_6_w );

MACHINE_DRIVER_EXTERN( invaders_sound );
WRITE8_HANDLER( invaders_sh_port_1_w );
WRITE8_HANDLER( invaders_sh_port_2_w );

MACHINE_DRIVER_EXTERN( blueshrk_sound );
WRITE8_HANDLER( blueshrk_sh_port_w );

MACHINE_DRIVER_EXTERN( invad2ct_sound );
WRITE8_HANDLER( invad2ct_sh_port_1_w );
WRITE8_HANDLER( invad2ct_sh_port_2_w );
WRITE8_HANDLER( invad2ct_sh_port_3_w );
WRITE8_HANDLER( invad2ct_sh_port_4_w );



/*----------- defined in video/mw8080bw.c -----------*/

VIDEO_UPDATE( mw8080bw );

VIDEO_UPDATE( spcenctr );

VIDEO_UPDATE( phantom2 );
VIDEO_EOF( phantom2 );

VIDEO_UPDATE( invaders );
