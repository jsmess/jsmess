/*****************************************************************************
 *
 * includes/svision.h
 *
 ****************************************************************************/

#ifndef SVISION_H_
#define SVISION_H_



class svision_state : public driver_device
{
public:
	svision_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in drivers/svision.c -----------*/

void svision_irq( running_machine *machine );


/*----------- defined in audio/svision.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(SVISION, svision_sound);

int *svision_dma_finished(running_device *device);
void svision_sound_decrement(running_device *device);
void svision_soundport_w(running_device *device, int which, int offset, int data);
WRITE8_DEVICE_HANDLER( svision_sounddma_w );
WRITE8_DEVICE_HANDLER( svision_noise_w );


#endif /* SVISION_H_ */
