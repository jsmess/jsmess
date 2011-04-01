/*****************************************************************************
 *
 * includes/pokemini.h
 *
 ****************************************************************************/

#ifndef POKEMINI_H_
#define POKEMINI_H_

typedef struct
{
	UINT8		colors_inverted;
	UINT8		background_enabled;
	UINT8		sprites_enabled;
	UINT8		copy_enabled;
	UINT8		map_size;
	UINT8		map_size_x;
	UINT8		frame_count;
	UINT8		max_frame_count;
	UINT32		bg_tiles;
	UINT32		spr_tiles;
	UINT8		count;
	emu_timer	*count_timer;
} PRC;


typedef struct
{
	emu_timer	*seconds_timer;
	emu_timer	*hz256_timer;
	emu_timer	*timer1;				/* Timer 1 low or 16bit */
	emu_timer	*timer1_hi;				/* Timer 1 hi */
	emu_timer	*timer2;				/* Timer 2 low or 16bit */
	emu_timer	*timer2_hi;				/* Timer 2 high */
	emu_timer	*timer3;				/* Timer 3 low or 16bit */
	emu_timer	*timer3_hi;				/* Timer 3 high */
} TIMERS;


class pokemini_state : public driver_device
{
public:
	pokemini_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	UINT8 m_pm_reg[0x100];
	PRC m_prc;
	TIMERS m_timers;
};


/*----------- defined in machine/pokemini.c -----------*/

MACHINE_START( pokemini );
WRITE8_DEVICE_HANDLER( pokemini_hwreg_w );
READ8_DEVICE_HANDLER( pokemini_hwreg_r );

DEVICE_IMAGE_LOAD( pokemini_cart );

#endif /* POKEMINI_H */
