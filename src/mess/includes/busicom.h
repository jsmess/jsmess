/*****************************************************************************
 *
 * includes/busicom.h
 *
 ****************************************************************************/

#ifndef BUSICOM_H_
#define BUSICOM_H_


class busicom_state : public driver_device
{
public:
	busicom_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 drum_index;
	UINT16 keyboard_shifter;
	UINT32 printer_shifter;
	UINT8 timer;
	UINT8 printer_line[11][17];
	UINT8 printer_line_color[11];
};


/*----------- defined in video/busicom.c -----------*/

extern PALETTE_INIT( busicom );
extern VIDEO_START( busicom );
extern VIDEO_UPDATE( busicom );

#endif /* BUSICOM_H_ */
