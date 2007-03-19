/***************************************************************************

  machine/spiders.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "spiders.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"



/* Declare inividual irq handlers, may be able to combine later */
/* Main CPU */
void spiders_irq0a(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }
void spiders_irq0b(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }
void spiders_irq1a(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }
void spiders_irq1b(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }
void spiders_irq2a(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }
void spiders_irq2b(int state) { cpunum_set_input_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }

/* Sound CPU */
void spiders_irq3a(int state) { cpunum_set_input_line(1,M6802_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE); }

static WRITE8_HANDLER( spiders_soundcmd_w )
{
	pia_set_input_a(3, data & 0xf8);
	pia_set_input_ca1(3,data  & 0x80 ? 1 : 0);
}

static WRITE8_HANDLER( spiders_sounda_w )
{
	discrete_sound_w(SPIDER_WEB_SOUND_MOD_DATA, 1 + (data & 4) * 8 + (data & 2) * 4 + (data & 1) * 2);
}

static WRITE8_HANDLER( spiders_soundb_w )
{
	discrete_sound_w(SPIDERS_WEB_SOUND_DATA, data);
}


static WRITE8_HANDLER( spiders_soundctrl_w )
{
	discrete_sound_w(SPIDERS_FIRE_EN, data & 0x10 ? 1 : 0);
	discrete_sound_w(SPIDERS_EXP_EN, data & 0x08 ? 1 : 0);
	discrete_sound_w(SPIDERS_SUPER_WEB_EXPL_EN, data & 0x04 ? 1 : 0);
	discrete_sound_w(SPIDERS_SUPER_WEB_EN, data & 0x02 ? 1 : 0);
	discrete_sound_w(SPIDERS_X_EN, data & 0x01 ? 1 : 0);

}

/* Function prototypes */

WRITE8_HANDLER( spiders_flip_w );
WRITE8_HANDLER( spiders_vrif_w );
READ8_HANDLER( spiders_vrom_r );


/* Declare PIA structure */

/* PIA 1, main CPU */
static const pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ spiders_irq0a, spiders_irq0b
};

/* PIA 2, main CPU */
static const pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ spiders_vrom_r, 0, input_port_5_r, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, spiders_vrif_w, 0, spiders_flip_w,
	/*irqs   : A/B             */ spiders_irq1a, spiders_irq1b
};

/* PIA 3, main CPU */
static const pia6821_interface pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ spiders_soundctrl_w, spiders_soundcmd_w, 0, 0,
	/*irqs   : A/B             */ spiders_irq2a, spiders_irq2b
};

/* PIA, sound CPU */
static const pia6821_interface pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ spiders_sounda_w, spiders_soundb_w, 0, 0,
	/*irqs   : A/B             */ spiders_irq3a, 0
};


/***************************************************************************

    Spiders Machine initialisation

***************************************************************************/

MACHINE_START( spiders )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia_0_intf);
	pia_config(1, PIA_ALTERNATE_ORDERING, &pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &pia_2_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &pia_3_intf);
	return 0;
}


MACHINE_RESET( spiders )
{
	pia_reset();
}



/***************************************************************************

    Timed interrupt handler - Updated PIA input ports

***************************************************************************/

INTERRUPT_GEN( spiders_timed_irq )
{
	/* Update CA1 on PIA1 - copy of PA1 (COIN1) */
	pia_0_ca1_w(0 , input_port_0_r(0)&0x02);

	/* Update CA2 on PIA1 - copy of PA0 (SERVICE1) */
	pia_0_ca2_w(0 , input_port_0_r(0)&0x01);

	/* Update CB1 on PIA1 - (Crosshatch) */
	pia_0_cb1_w(0 , input_port_6_r(0));

	/* Update CB2 on PIA1 - NOT CONNECTED */
}


/***************************************************************************

    Video access port definition (On PIA 2)

    Bit 7 6 5 4 3 2 1 0
        X                Mode Setup/Read 1/0
            X X          Latch select (see below)
                X X X X  Data nibble

    When in setup mode data is clocked into the latch by a read from port a.
    When in read mode the read from port a auto increments the address.

    Latch 0 - Low byte low nibble
          1 - Low byte high nibble
          2 - High order low nibble
          3 - High order high nibble

***************************************************************************/

static int vrom_address;
static int vrom_ctrl_mode;
static int vrom_ctrl_latch;
static int vrom_ctrl_data;

int spiders_video_flip=0;

WRITE8_HANDLER( spiders_flip_w )
{
	spiders_video_flip=data;
}

WRITE8_HANDLER( spiders_vrif_w )
{
	vrom_ctrl_mode=(data&0x80)>>7;
	vrom_ctrl_latch=(data&0x30)>>4;
	vrom_ctrl_data=15-(data&0x0f);
}

READ8_HANDLER( spiders_vrom_r )
{
	int retval;
	unsigned char *RAM = memory_region(REGION_GFX1);

	if(vrom_ctrl_mode)
	{
		retval=RAM[vrom_address];
		vrom_address++;
	}
	else
	{
		switch(vrom_ctrl_latch)
		{
			case 0:
				vrom_address&=0xfff0;
				vrom_address|=vrom_ctrl_data;
				break;
			case 1:
				vrom_address&=0xff0f;
				vrom_address|=vrom_ctrl_data<<4;
				break;
			case 2:
				vrom_address&=0xf0ff;
				vrom_address|=vrom_ctrl_data<<8;
				break;
			case 3:
				vrom_address&=0x0fff;
				vrom_address|=vrom_ctrl_data<<12;
				break;
			default:
				break;
		}
		retval=0;
	}
	return retval;
}

