#ifndef _CPSCHNGHR_H_
#define _CPSCHNGHR_H_


class cpschngr_state : public driver_device
{
public:
	cpschngr_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_qsound_sharedram1;
	UINT8 *m_qsound_sharedram2;
	UINT16 *m_cps1_gfxram;
	UINT16 *m_cps1_cps_a_regs;
	UINT16 *m_cps1_cps_b_regs;
	size_t m_cps1_gfxram_size;
	UINT16 *m_cps1_scroll1;
	UINT16 *m_cps1_scroll2;
	UINT16 *m_cps1_scroll3;
	UINT16 *m_cps1_obj;
	UINT16 *m_cps1_buffered_obj;
	UINT16 *m_cps1_other;
	int m_cps1_last_sprite_offset;
	int m_cps1_stars_enabled[2];
	tilemap_t *m_cps1_bg_tilemap[3];
	int m_cps1_scroll1x;
	int m_cps1_scroll1y;
	int m_cps1_scroll2x;
	int m_cps1_scroll2y;
	int m_cps1_scroll3x;
	int m_cps1_scroll3y;
	int m_stars1x;
	int m_stars1y;
	int m_stars2x;
	int m_stars2y;
	const struct CPS1config *m_cps1_game_config;
	UINT8 m_empty_tile8x8[8*8];
	UINT8 m_empty_tile[32*32/2];
};


/*----------- defined in machine/kabuki.c -----------*/

void wof_decode(running_machine &machine);


/*----------- defined in video/cpschngr.c -----------*/

WRITE16_HANDLER( cps1_cps_a_w );
WRITE16_HANDLER( cps1_cps_b_w );
READ16_HANDLER( cps1_cps_b_r );
WRITE16_HANDLER( cps1_gfxram_w );

DRIVER_INIT( cps1 );

VIDEO_START( cps1 );
SCREEN_UPDATE( cps1 );
SCREEN_EOF( cps1 );

#endif
