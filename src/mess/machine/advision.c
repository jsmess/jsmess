/***************************************************************************

  advision.c

  Machine file to handle emulation of the AdventureVision.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/advision.h"
#include "devices/cartslot.h"
#include "image.h"
#include "cpuintrf.h"
#include "sound/dac.h"
#include "cpuint.h"

unsigned char *advision_ram;
int advision_rambank;
int advision_framestart;
int advision_videoenable;
int advision_videobank;
static int Lvalue[3];
static int wLpointer;
static int rLpointer;
static int Dvalue;
static int Gvalue;

MACHINE_RESET( advision )
{
	advision_ram = memory_region(REGION_CPU1) + 0x2000;
    advision_rambank = 0x300;
    memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x1000);
    advision_framestart = 0;
    advision_videoenable = 0;
    wLpointer = 0;
	rLpointer = 0;
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);

}

/****** External RAM ******************************/

 READ8_HANDLER ( advision_MAINRAM_r )
{
    int d;

    d = advision_ram[advision_rambank + offset];

	/* the video hardware interprets reads as writes */
    if (!advision_videoenable) advision_vh_write(d);

     if (advision_videobank == 0x06) {
		    if (d & 0x01) {
	   			cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
	   		/*	logerror("RELEASE RESET\n"); */
				wLpointer=0;
				rLpointer=0;
			}
			else {
				cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
			/*	logerror("SET RESET\n");*/
			}

		}

    return d;
}

WRITE8_HANDLER ( advision_MAINRAM_w )
{
    advision_ram[advision_rambank + offset] = data;
}


READ8_HANDLER ( advision_getL )
{
	int d = 0;

	d = Lvalue[rLpointer];
	if (rLpointer < 3 ) rLpointer++;

/*	if (rLpointer == 0) {d = 0x0f; }
	if (rLpointer == 1) {d = 0x01; }
	rLpointer++;

	if (rLpointer > 1) rLpointer = 0; */

	/* logerror("READ L: %x\n",d); */

	return d;
}

static void update_dac(void) {
/*	logerror("Clock: %x D: %x  G:%x \n",activecpu_get_icount(),Dvalue, Gvalue); */

  	if (Gvalue == 0 && Dvalue == 0)
		DAC_data_w(0, 0xff);
   	else if (Gvalue == 1 && Dvalue == 1)
   		DAC_data_w(0, 0x80);
   	else
		DAC_data_w(0, 0x00);
}

WRITE8_HANDLER ( advision_putG )
{
	Gvalue = data & 0x01;
	update_dac();
}

WRITE8_HANDLER ( advision_putD )
{
	Dvalue = data & 0x01;
	update_dac();
}


static void sound_write(int data)
{
	Lvalue[wLpointer] = ((data & 0xF0) >> 4);
	if (wLpointer < 3) wLpointer++;
	/* logerror("WRITE L: %x\n",data); */
}

/***** 8048 Ports ************************/

WRITE8_HANDLER ( advision_putp1 )
{
	static UINT8 *ROM;

	ROM = memory_region(REGION_CPU1);
	if (data & 0x04)
		memory_set_bankptr(1,&ROM[0x0000]);
	else
		memory_set_bankptr(1,&ROM[0x1000]);
	advision_rambank = (data & 0x03) << 8;
}

WRITE8_HANDLER ( advision_putp2 )
{

	sound_write(data);

	if ((advision_videoenable == 0x00) && (data & 0x10))
	{
		advision_vh_update(advision_vh_hpos);
		advision_vh_hpos++;
		if (advision_vh_hpos > 255)
		{
			advision_vh_hpos = 0;
			logerror("HPOS OVERFLOW\n");
		}
	}
	advision_videoenable = data & 0x10;
	advision_videobank = (data & 0xE0) >> 5;
}

 READ8_HANDLER ( advision_getp1 ) {
    int d,in;

    // Get joystick switches
    in = input_port_0_r(0);
    d = in | 0x0F;

    // Get buttons
    if (in & 0x02) d = d & 0xF7;    /* Button 3 */
    if (in & 0x08) d = d & 0xCF;    /* Button 1 */
    if (in & 0x04) d = d & 0xAF;    /* Button 2 */
    if (in & 0x01) d = d & 0x6F;    /* Button 4 */

    d = d & 0xF8;
    return d;
}

 READ8_HANDLER ( advision_getp2 ) {
    return 0;
}

 READ8_HANDLER ( advision_gett0 ) {
    return 0;
}

 READ8_HANDLER ( advision_gett1 ) {
    if (advision_framestart) {
        advision_framestart = 0;
        return 0;
    }
    else {
        return 1;
    }
}
