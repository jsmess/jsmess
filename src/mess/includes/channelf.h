/*****************************************************************************
 *
 * includes/channelf.h
 *
 ****************************************************************************/

#ifndef CHANNELF_H_
#define CHANNELF_H_

/* SKR - 2102 RAM chip on carts 10 and 18 I/O ports */
typedef struct
{
	UINT8 d;			/* data bit:inverted logic, but reading/writing cancel out */
	UINT8 r_w;			/* inverted logic: 0 means read, 1 means write */
	UINT8 a[10];		/* addr bits: inverted logic, but reading/writing cancel out */
	UINT16 addr;		/* calculated addr from addr bits */
	UINT8 ram[1024];	/* RAM array */
} r2102_t;


class channelf_state : public driver_device
{
public:
	channelf_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	UINT8 latch[6];
	r2102_t r2102;
	UINT8 val_reg;
	UINT8 row_reg;
	UINT8 col_reg;
};


/*----------- defined in video/channelf.c -----------*/

PALETTE_INIT( channelf );
VIDEO_START( channelf );
VIDEO_UPDATE( channelf );


/*----------- defined in audio/channelf.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(CHANNELF, channelf_sound);

void channelf_sound_w(running_device *device, int mode);


#endif /* CHANNELF_H_ */
