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

typedef struct
{
	UINT8 reg[5];
	int on, right, left;
	int ca14to16;
	int start,size;
	double pos, step;
	int finished;
} SVISION_DMA;

extern SVISION_DMA svision_dma;

typedef enum
{
	SVISION_NOISE_Type7Bit,
	SVISION_NOISE_Type14Bit
} SVISION_NOISE_Type;

typedef struct
{
	UINT8 reg[3];
	int on, right, left, play;
	SVISION_NOISE_Type type;
	int state;
	int volume;
	int count;
	double step, pos;
	int value; // currently simple random function
} SVISION_NOISE;

extern SVISION_NOISE svision_noise;

typedef struct
{
	UINT8 reg[4];
	int on;
	int waveform, volume;
	int pos;
	int size;
	int count;
} SVISION_CHANNEL;

extern SVISION_CHANNEL svision_channel[2];

DECLARE_LEGACY_SOUND_DEVICE(SVISION, svision_sound);
void svision_soundport_w (running_machine *machine, SVISION_CHANNEL *channel, int offset, int data);
WRITE8_HANDLER( svision_sounddma_w );
WRITE8_HANDLER( svision_noise_w );


#endif /* SVISION_H_ */
