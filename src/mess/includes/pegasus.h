class pegasus_state : public driver_device
{
public:
	pegasus_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 kbd_row;
	UINT8 kbd_irq;
	running_device *cass;
	UINT8 *FNT;
	UINT8 control_bits;
	UINT8 *video_ram;
};


/*----------- defined in video/pegasus.c -----------*/

VIDEO_UPDATE( pegasus );
