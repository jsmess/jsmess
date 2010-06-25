/***************************************************************************

    toshiba 6721a chip approximation
    (voice output)

    not really working
     communication with c364 works, no speech synthesis
    includes c364 interface hardware

    PeT mess@utanet.at
    documentation
     www.funet.fi

***************************************************************************/

/*
 c364 speech
 say 0 .. 10
 rate 0 .. 15?
 voc ?
 rdy ? (only c64)

 0 bit 0..3 ???
   bit 456 0?
   bit 7 writen 0 1
   reset 9 9 b
   set playback rate
    rate 4: 2 a 4 5 4 6 0 7 a (default)
         0          0
         1          1
    rate 2: 2 a 4 5 2 6 0 7 a
    rate 3:         3
    rate 9:
   start: 1
 1 bit 01 set to 1 for start ?
   bit 6 polled until set (at $80ec)
       7 set ready to transmit new byte?
 2 0..7 sample data

seems to be a toshiba t6721a build in
(8 kHz 9bit output)
generates output for 20ms (or 10ms) out of 6 byte voice data!
(P?ARCOR voice synthesizing and analyzing method
 Nippon Telegraph and Telephon Public Corporation)
End code also in voice data
technical info at www.funet.fi covers only the chip, not
the synthesizing method

magic voice in c64:
The internal electronics depicted in Danny's picture above are as follows, going from the MOS chip at top and then clockwise: MOS
6525B (4383), MOS 251476-01 (8A-06 4341) system ROM, General Instruments 8343SEA (LA05-123), Toshiba T6721A (3L)
sound generator (?), CD40105BE (RCA H 432) and a 74LS222A logic chip.

*/

#include "emu.h"
#include "audio/t6721.h"


typedef struct _t6721_state  t6721_state;
struct _t6721_state
{
	emu_timer *timer;

	int busy, end_of_sample;
	int playing;
	int rate;

	UINT8 command_data;
	int command_state;

	UINT8 sample_data[6], sample_index;

	UINT8 state;

	int sample_timeindex;
	UINT8 readindex, writeindex;
	UINT64 data[0x10];
};

/*****************************************************************************
    LOGGING
*****************************************************************************/

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(device->machine)), (char*) M ); \
			logerror A; \
		} \
	} while(0)


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE t6721_state *get_safe_token( running_device *device )
{
	assert(device != NULL);
	assert(device->type() == T6721);

	return (t6721_state *)downcast<legacy_device_base *>(device)->token();
}

/*****************************************************************************
    IMPLEMENTATION
*****************************************************************************/

static TIMER_CALLBACK( t6721_speech_timer )
{
	t6721_state *t6721 = (t6721_state *)ptr;

	if (!t6721->playing)
		return;

	if (t6721->sample_timeindex < 8000 / 50)
	{
		t6721->sample_timeindex++;
	}
	else
	{
		t6721->end_of_sample = (memcmp(t6721->sample_data, "\xff\xff\xff\xff\xff\xff", 6) == 0);

		/*t6721->end_of_sample = 1; */
		t6721->busy = 0;
	}
}

WRITE8_DEVICE_HANDLER( t6721_speech_w )
{
	t6721_state *t6721 = get_safe_token(device);

	DBG_LOG(2, "364", ("port write %.2x %.2x\n", offset, data));

	switch (offset)
	{
	case 0:
		if (data & 0x80)
		{
			switch (t6721->command_state)
			{
			case 0:
				switch (t6721->command_data)
				{
				case 9: case 0xb:
					t6721->playing = 0;
					break;
				case 1: /* start */
					timer_adjust_periodic(t6721->timer, attotime_zero, 0, ATTOTIME_IN_HZ(8000));
					t6721->playing = 1;
					t6721->end_of_sample = 0;
					t6721->sample_timeindex = 0;
					break;
				case 2:
					t6721->end_of_sample = 0;
					/*t6721->busy = 0; */
					timer_reset(t6721->timer, attotime_never);
					t6721->playing = 0;
					break;
				case 5: /* set rate (in next nibble) */
					t6721->command_state = 1;
					break;
				case 6: /* condition */
					t6721->command_state = 2;
					break;
				}
				break;
			case 1:
				t6721->command_state = 0;
				t6721->rate = t6721->command_data;
				break;
			case 2:
				t6721->command_state = 0;
				break;
			}
		}
		else
		{
			t6721->command_data = data;
		}
		break;
	case 1:
		t6721->state = (t6721->state & ~0x3f) | data;
		break;
	case 2:
		t6721->sample_data[t6721->sample_index++] = data;
		if (t6721->sample_index == sizeof(t6721->sample_data))
		{
			DBG_LOG(1,"t6721",("%.2x%.2x%.2x%.2x%.2x%.2x\n",
							   t6721->sample_data[0],
							   t6721->sample_data[1],
							   t6721->sample_data[2],
							   t6721->sample_data[3],
							   t6721->sample_data[4],
							   t6721->sample_data[5]));
			t6721->sample_index = 0;
			/*t6721->end_of_sample = false; */
			t6721->busy = 1;
			t6721->state = 0;
		}
		break;
	}
}

READ8_DEVICE_HANDLER( t6721_speech_r )
{
	t6721_state *t6721 = get_safe_token(device);

	int data = 0xff;
	switch (offset)
	{
	case 1:
		data = t6721->state;
		data = 1;
		if (!t6721->end_of_sample)
		{
				data |= 0x41;
				if (!t6721->busy)
					data |= 0x81;
		}
		break;
	}
	DBG_LOG(2, "364", ("port read %.2x %.2x\n", offset, data));

	return data;
}

/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( t6721 )
{
	t6721_state *t6721 = get_safe_token(device);

	t6721->timer = timer_alloc(device->machine, t6721_speech_timer, t6721);

	state_save_register_device_item_array(device, 0, t6721->sample_data);
	state_save_register_device_item_array(device, 0, t6721->data);

	state_save_register_device_item(device, 0, t6721->sample_index);
	state_save_register_device_item(device, 0, t6721->busy);

	state_save_register_device_item(device, 0, t6721->end_of_sample);
	state_save_register_device_item(device, 0, t6721->playing);
	state_save_register_device_item(device, 0, t6721->rate);

	state_save_register_device_item(device, 0, t6721->command_data);
	state_save_register_device_item(device, 0, t6721->command_state);
	state_save_register_device_item(device, 0, t6721->state);

	state_save_register_device_item(device, 0, t6721->sample_timeindex);

	state_save_register_device_item(device, 0, t6721->readindex);
	state_save_register_device_item(device, 0, t6721->writeindex);
}


static DEVICE_RESET( t6721 )
{
	t6721_state *t6721 = get_safe_token(device);

	memset(t6721->sample_data, 0, ARRAY_LENGTH(t6721->sample_data));
	memset(t6721->data, 0, ARRAY_LENGTH(t6721->data));

	t6721->sample_index = 0;
	t6721->busy = 0;

	t6721->end_of_sample = 0;
	t6721->playing = 0;
	t6721->rate = 0;

	t6721->command_data = 0;
	t6721->command_state = 0;
	t6721->state = 0;

	t6721->sample_timeindex = 0;

	t6721->readindex = 0;
	t6721->writeindex = 0;
}


/*-------------------------------------------------
    device definition
-------------------------------------------------*/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##t6721##s
#define DEVTEMPLATE_FEATURES			DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME				"Toshiba 6721A"
#define DEVTEMPLATE_FAMILY				"Toshiba 6721A"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(T6721, t6721);
