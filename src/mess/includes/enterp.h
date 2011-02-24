#ifndef __ENTERP_H__
#define __ENTERP_H__


/* there are 64us per line, although in reality
   about 50 are visible. */
#define ENTERPRISE_SCREEN_WIDTH (50*16)

/* there are 312 lines per screen, although in reality
   about 35*8 are visible */
#define ENTERPRISE_SCREEN_HEIGHT	(35*8)


#define NICK_PALETTE_SIZE	256


class ep_state : public driver_device
{
public:
	ep_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 exdos_card_value;  /* state of the wd1770 irq/drq lines */
	UINT8 keyboard_line;     /* index of keyboard line to read */
	struct _NICK_STATE *nick;
};


/*----------- defined in video/epnick.c -----------*/

PALETTE_INIT( epnick );
VIDEO_START( epnick );
SCREEN_UPDATE( epnick );

WRITE8_HANDLER( epnick_reg_w );


#endif /* __ENTERP_H__ */
