/*****************************************************************************
 *
 * includes/svision.h
 *
 ****************************************************************************/

#ifndef SVISION_H_
#define SVISION_H_

typedef struct
{
	emu_timer *timer1;
	int timer_shot;
} svision_t;

typedef struct
{
	int state;
	int on, clock, data;
	UINT8 input;
	emu_timer *timer;
} svision_pet_t;

typedef struct
{
	UINT16 palette[4/*0x40?*/]; /* rgb8 */
	int palette_on;
} tvlink_t;


class svision_state : public driver_device
{
public:
	svision_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	UINT8 *m_reg;
	device_t *m_sound;
	int *m_dma_finished;
	svision_t m_svision;
	svision_pet_t m_pet;
	tvlink_t m_tvlink;
};


/*----------- defined in drivers/svision.c -----------*/

void svision_irq( running_machine &machine );


/*----------- defined in audio/svision.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(SVISION, svision_sound);

int *svision_dma_finished(device_t *device);
void svision_sound_decrement(device_t *device);
void svision_soundport_w(device_t *device, int which, int offset, int data);
WRITE8_DEVICE_HANDLER( svision_sounddma_w );
WRITE8_DEVICE_HANDLER( svision_noise_w );


#endif /* SVISION_H_ */
