/*****************************************************************************
 *
 * includes/channelf.h
 *
 ****************************************************************************/

#ifndef CHANNELF_H_
#define CHANNELF_H_



class channelf_state : public driver_device
{
public:
	channelf_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in video/channelf.c -----------*/

extern UINT8 channelf_val_reg;
extern UINT8 channelf_row_reg;
extern UINT8 channelf_col_reg;

PALETTE_INIT( channelf );
VIDEO_START( channelf );
VIDEO_UPDATE( channelf );


/*----------- defined in audio/channelf.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(CHANNELF, channelf_sound);

void channelf_sound_w(running_device *device, int mode);


#endif /* CHANNELF_H_ */
