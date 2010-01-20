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

#include <ctype.h>
#include "emu.h"
#include "cpu/m6502/m6502.h"

#include "includes/c16.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)



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
static struct {
	emu_timer *timer;

	int busy, endOfSample;
	int playing;
	int rate;
	struct {
		UINT8 data;
		int state;
	} command;
	struct {
		UINT8 data[6],index;
	} sample;

	UINT8 state;

	int sampleindex;
	UINT8 readindex, writeindex;
	UINT64 data[0x10];

} speech={ 0 };

static TIMER_CALLBACK(c364_speech_timer)
{
	if (!speech.playing) return;

	if (speech.sampleindex<8000/50) {
		speech.sampleindex++;
	} else {
		speech.endOfSample=
				(memcmp(speech.sample.data,"\xff\xff\xff\xff\xff\xff",6)==0);
		/*speech.endOfSample=TRUE; */
		speech.busy=FALSE;
	}
}

void c364_speech_init(running_machine *machine)
{
	speech.timer = timer_alloc(machine, c364_speech_timer, NULL);
}

void c364_speech_reset(running_machine *machine)
{
	memset(&speech, 0, sizeof(speech));
}

WRITE8_HANDLER(c364_speech_w)
{
	running_machine *machine = space->machine;
	DBG_LOG (2, "364", ("port write %.2x %.2x\n", offset, data));
	switch (offset) {
	case 0:
		if (data&0x80) {
			switch (speech.command.state) {
			case 0:
				switch (speech.command.data) {
				case 9:case 0xb:
					speech.playing=FALSE;
					break;
				case 1: /* start */
					timer_adjust_periodic(speech.timer, attotime_zero, 0, ATTOTIME_IN_HZ(8000));
					speech.playing=TRUE;
					speech.endOfSample=FALSE;
					speech.sampleindex=0;
					break;
				case 2:
					speech.endOfSample=FALSE;
					/*speech.busy=FALSE; */
					timer_reset(speech.timer, attotime_never);
					speech.playing=FALSE;
					break;
				case 5: /* set rate (in next nibble) */
					speech.command.state=1;
					break;
				case 6: /* condition */
					speech.command.state=2;
					break;
				}
				break;
			case 1:
				speech.command.state=0;
				speech.rate=speech.command.data;
				break;
			case 2:
				speech.command.state=0;
				break;
			}
		} else {
			speech.command.data=data;
		}
		break;
	case 1:
		speech.state=(speech.state&~0x3f)|data;
		break;
	case 2:
		speech.sample.data[speech.sample.index++]=data;
		if (speech.sample.index==sizeof(speech.sample.data)) {
			DBG_LOG(1,"t6721",("%.2x%.2x%.2x%.2x%.2x%.2x\n",
							   speech.sample.data[0],
							   speech.sample.data[1],
							   speech.sample.data[2],
							   speech.sample.data[3],
							   speech.sample.data[4],
							   speech.sample.data[5]));
			speech.sample.index=0;
			/*speech.endOfSample=false; */
			speech.busy=TRUE;
			speech.state=0;
		}
		break;
	}
}

READ8_HANDLER(c364_speech_r)
{
	running_machine *machine = space->machine;
	int data=0xff;
	switch (offset) {
	case 1:
		data=speech.state;
		data=1;
		if (!speech.endOfSample) {
				data|=0x41;
				if (!speech.busy) data |=0x81;
		}
		break;
	}
	DBG_LOG (2, "364", ("port read %.2x %.2x\n", offset, data));
	return data;
}


