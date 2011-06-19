/////////////////////////////////////////////////////////////////////////
// HEC2HRP.C  in machine
/*      Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector 1
        Interact

        12/05/2009 Skeleton driver - Micko : mmicko@gmail.com
        31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                        by yo_fr       (jj.stac @ aliceadsl.fr)

                => add Keyboard,
                => add color,
                => add cassette,
                => add sn76477 sound and 1bit sound,
                => add joysticks (stick, pot, fire)
                => add BR/HR switching
                => add bank switch for HRX
                => add device MX80c and bank switching for the ROM
        03/01/2010 Update and clean prog  by yo_fr       (jj.stac@aliceadsl.fr)
                => add the port mapping for keyboard
        28/09/2010 add the DISK II support by yo_fr      (jj.stac@aliceadsl.fr)
                => Note that actually the DISK II boot (loading CPM : OK) but do not run (don't run the CPM...).
        20/11/2010 : synchronization between uPD765 and Z80 are now OK, CP/M runnig! JJStacino

    don't forget to keep some information about these machines, see DChector project : http://dchector.free.fr/ made by DanielCoulom
        (and thank's to Daniel!)

    TODO :  Add the cartridge function,
            Adjust the one shot and A/D timing (sn76477)
*/

#include "emu.h"
#include "imagedev/cassette.h"
#include "imagedev/printer.h"
#include "sound/sn76477.h"   /* for sn sound*/
#include "sound/wave.h"      /* for K7 sound*/
#include "sound/discrete.h"  /* for 1 Bit sound*/
#include "machine/upd765.h"	/* for floppy disc controller */
#include "formats/basicdsk.h"

#include "formats/hect_tap.h"
#include "includes/hec2hrp.h"

#ifndef DEBUG_TRACE_COM_HECTOR
//#define DEBUG_TRACE_COM_HECTOR  1
#endif

static void Mise_A_Jour_Etat(running_machine &machine, int Adresse, int Value );
static void Update_Sound(address_space *space, UINT8 data);

static cassette_image_device *cassette_device_image(running_machine &machine);

/* Cassette timer*/
static TIMER_CALLBACK( Callback_CK )
{
	hec2hrp_state *state = machine.driver_data<hec2hrp_state>();
/* To generate the CK signal (K7)*/
	state->m_CK_signal++;
}

WRITE8_HANDLER( hector_switch_bank_w )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	if (offset==0x00)	{	/* 0x800 et 0x000=> video page, HR*/
							memory_set_bank(space->machine(), "bank1", HECTOR_BANK_VIDEO);
							if (state->m_flag_clk ==1)
							{
								state->m_flag_clk=0;
								space->machine().device("maincpu")->set_unscaled_clock(XTAL_5MHz);  /* Augmentation CPU*/
							}
						}
	if (offset==0x04)	{	/* 0x804 => video page, BR*/
							state->m_hector_flag_hr=0;
							memory_set_bank(space->machine(), "bank1", HECTOR_BANK_VIDEO);
							if (state->m_flag_clk ==0)
							{
								state->m_flag_clk=1;
								space->machine().device("maincpu")->set_unscaled_clock(XTAL_1_75MHz);  /* Ralentissement CPU*/
							}
						}
	if (offset==0x08)	{	/* 0x808 => base page, HR*/
							memory_set_bank(space->machine(), "bank1", HECTOR_BANK_PROG);
							if (state->m_flag_clk ==1)
							{
								state->m_flag_clk=0;
								space->machine().device("maincpu")->set_unscaled_clock(XTAL_5MHz);  /* Augmentation CPU*/
							}

						}
	if (offset==0x0c)	{	/* 0x80c => base page, BR*/
							state->m_hector_flag_hr=0;
							memory_set_bank(space->machine(), "bank1", HECTOR_BANK_PROG);
							if (state->m_flag_clk ==0)
							{
								state->m_flag_clk=1;
								space->machine().device("maincpu")->set_unscaled_clock(XTAL_1_75MHz);  /* Ralentissement CPU*/
							}
						}
}

WRITE8_HANDLER( hector_keyboard_w )
{
	/*nothing to do => read function manage the value*/
}

READ8_HANDLER( hector_keyboard_r )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	UINT8 data = 0xff;
	running_machine &machine = space->machine();

	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8" };

	if (offset ==7) /* Only when joy reading*/
	{
		/* Read special key for analog joystick emulation only (button and pot are analog signal!) and the reset */
		data=input_port_read(machine, keynames[8]);

		if (data & 0x01) /* Reset machine ! (on ESC key)*/
		{
		  cputag_set_input_line(machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
		  state->m_hector_flag_hr=1;
		if (strncmp(machine.system().name , "hec2hrx"  , 7)==0 ||
				strncmp(machine.system().name , "hec2mx40" , 8)==0 ||
				strncmp(machine.system().name , "hec2mx80" , 8)==0   ) /* aviable for HRX and up */
			{
				memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);
				memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
				hector_disc2_reset(machine);

			}
		if (strncmp(machine.system().name , "hector1"  , 7)==0 ||
				strncmp(machine.system().name , "interact" , 8)==0   ) /* aviable for BR machines */

					state->m_hector_flag_hr=0;

		/*Common flag*/
		state->m_hector_flag_80c = 0;
		state->m_flag_clk = 0;
		}

		state->m_actions = 0;
		if (data & 0x02) /* Fire(0)*/
			state->m_actions += 1;

		if (data & 0x04) /* Fire(1)*/
			state->m_actions += 2;

		if (data & 0x08) /* Pot(0)+*/
			state->m_pot0 += 1;
		if (state->m_pot0>128) state->m_pot0 = 128;

		if (data & 0x10) /* Pot(0)-*/
			state->m_pot0 -= 1;

		if (state->m_pot0>250) state->m_pot0 = 0;

		if (data & 0x20) /* Pot(1)+*/
			state->m_pot1 += 1;
		if (state->m_pot1>128) state->m_pot1 = 128;

		if (data & 0x40) /* Pot(1)-*/
			state->m_pot1 -= 1;

		if (state->m_pot1>250) state->m_pot1 = 0;
	}

	/* in all case return the request value*/
	return input_port_read(space->machine(), keynames[offset]);
}

WRITE8_HANDLER( hector_sn_2000_w )
{
	Mise_A_Jour_Etat(space->machine(), 0x2000+ offset, data);
	Update_Sound(space, data);
}
WRITE8_HANDLER( hector_sn_2800_w )
{
	Mise_A_Jour_Etat(space->machine(), 0x2800+ offset, data);
	Update_Sound(space, data);
}
READ8_HANDLER( hector_cassette_r )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	double level;
	UINT8 value=0;

	if ((state->m_state3000 & 0x38) != 0x38 )   /* Selon Sb choix cassette ou timer (74153)*/
	{
		state->m_Data_K7 =  0x00;  /* No cassette => clear bit*/
		switch (state->m_state3000 & 0x38 )
		{
			case 0x08: value = (state->m_actions & 1) ? 0x80 : 0; break;
			case 0x10: value = state->m_pot0; break;
			case 0x20: value = (state->m_actions & 2) ? 0x80 : 0; break;
			case 0x28: value = state->m_pot1; break;
			default: value = 0; break;
		}
	}
	else
	{
		if (state->m_write_cassette == 0)
		{
			/* Accee a la cassette*/
			level = (cassette_device_image(space->machine())->input());

			/* Travail du 741 en trigger*/
			if  (level < -0.08)
				state->m_cassette_bit = 0x00;
			if (level > +0.08)
				state->m_cassette_bit = 0x01;
		}
		/* Programme du sn7474 (bascule) : Changement ??tat bit Data K7 ?? chaque front montant de state->m_cassette_bit*/
		if ((state->m_cassette_bit != state->m_cassette_bit_mem) && (state->m_cassette_bit !=0))
		{
			if (state->m_Data_K7 == 0x00)
				state->m_Data_K7 =  0x80;/* En poids fort*/
			else
				state->m_Data_K7 =  0x00;
		}
		value = ( state->m_CK_signal & 0x7F ) + state->m_Data_K7;
		state->m_cassette_bit_mem = state->m_cassette_bit;  /* Memorisation etat bit cassette*/
	}
	return value;
}
WRITE8_HANDLER( hector_sn_3000_w )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	state->m_state3000 = data & 0xf8; /* except bit 0 to 2*/
	if ((data & 7) != state->m_oldstate3000 )
	{
		/* Update sn76477 only when necessary!*/
		Mise_A_Jour_Etat( space->machine(), 0x3000, data & 7 );
		Update_Sound(space, data & 7);
	}
	state->m_oldstate3000 = data & 7;
}

/* Color Interface */
WRITE8_HANDLER( hector_color_a_w )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();

	if (data & 0x40)
	{
		/* Bit 6 => motor ON/OFF => for cassette state!*/
		if (state->m_write_cassette==0)
			 cassette_device_image(space->machine())->set_state((cassette_state)(CASSETTE_PLAY | CASSETTE_SPEAKER_ENABLED));
	}
	else
	{	/* stop motor*/
		cassette_device_image(space->machine())->set_state(CASSETTE_STOPPED);
		state->m_write_cassette=0;
		state->m_counter_write =0;
	}
	if (((data & 0x80) != (state->m_oldstate1000 & 0x80)) && ((state->m_oldstate1000 & 7)==(data & 7)) ) /* Bit7 had change but not the color statement*/
	{
		/* Bit 7 => Write bit for cassette!*/
		state->m_counter_write +=1;

		if (state->m_counter_write > 5)
		{
			/* Wait several cycle before lauch the record to prevent somes bugs*/
			state->m_counter_write = 6;
			if (state->m_write_cassette==0)
			{	/* C'est la 1er fois => record*/
				cassette_device_image(space->machine())->set_state(CASSETTE_RECORD);
				state->m_write_cassette=1;
			}
		}
		/* cassette data */
		cassette_device_image(space->machine())->output(((data & 0x80) == 0x80) ? -1.0 : +1.0);
	}

	/* Other bit : color definition*/
	state->m_hector_color[0] =  data        & 0x07 ;
	state->m_hector_color[2] = ((data >> 3)  & 0x07) | (state->m_hector_color[2] & 0x40);

	state->m_oldstate1000=data; /* For next step*/
}

WRITE8_HANDLER( hector_color_b_w )
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	device_t *discrete = space->machine().device("discrete");
	state->m_hector_color[1] =  data        & 0x07;
	state->m_hector_color[3] = (data >> 3)  & 0x07;

	/* Half light on color 2 only on HR machines:*/
	if (data & 0x40) state->m_hector_color[2] |= 8; else state->m_hector_color[2] &= 7;

	/* Play bit*/
	discrete_sound_w(discrete, NODE_01,  (data & 0x80) ? 0:1 );
}


/******************************************************************************
 Cassette Handling
******************************************************************************/

static cassette_image_device *cassette_device_image(running_machine &machine)
{
	return machine.device<cassette_image_device>(CASSETTE_TAG);
}


/********************************************************************************
 Port Handling
********************************************************************************/

/*******************  READ PIO 8255 *******************/
READ8_HANDLER( hector_io_8255_r)
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();

	/* 8255 in mode 0 */
	UINT8 data =0;
	UINT8 data_l=0;
	UINT8 data_h=0;


	if ((offset & 0x3) == 0x0) /* Port A */
		data = state->m_hector_port_a;

	if ((offset & 0x3) == 0x1) /* Port B */
	{
		data = state->m_hector_port_b;
		#ifdef DEBUG_TRACE_COM_HECTOR
			printf("\nLecture data par Hector %x (portB)",data);
		#endif
	}

	if ((offset & 0x3) == 0x2) /* Port C */
	{
		data_l = (state->m_hector_port_c_l & 0x0f);
		data_h = (state->m_hector_port_c_h & 0xf0);

		if (BIT(state->m_hector_port_cmd, 0))					/* Quartet inf en entree ?*/
			data_l = (state->m_hector_port_c_l & 0x0f);  /*no change*/

		if (BIT(state->m_hector_port_cmd, 3))					/* Quartet sup en entree ?*/
		{
			state->m_hector_port_c_h = (state->m_hector_port_c_h & 0x0c0);	/* Clear bits 4 & 5*/

			if (state->m_hector_disc2_data_w_ready != 0x00)
				state->m_hector_port_c_h = state->m_hector_port_c_h + 0x010;	// PC4 (data write ready from Disc II to Hector)

			if (state->m_hector_disc2_data_r_ready != 0x00)
				state->m_hector_port_c_h = state->m_hector_port_c_h + 0x020;	// PC5 (data read ready from Hector to Disc2)

			state->m_hector_port_c_h = state->m_hector_port_c_h & 0x07F;		// PC7 (printer busy=0)
			data_h =  state->m_hector_port_c_h;
		}
		data= data_l + data_h;
	}
	return data;  // Return the value!
}

/*******************  WRITE PIO 8255 *******************/

WRITE8_HANDLER( hector_io_8255_w)
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	/* 8255 in mode 0 */
	if ((offset & 0x3) == 0x0) /* Port A => to printer or Disc II*/
	{
		state->m_hector_port_a = data;
		/* Port A => to printer*/
		/*  Caution : The strobe connection to the printer seems not be used
        So, all what is send to the Disc2 unit will be printed too! */

		if (BIT(state->m_hector_port_c_l, 0)) {		// PC0 (bit X0)= strobe printer !
			printer_image_device *printer = space->machine().device<printer_image_device>("printer");
			printer->output(state->m_hector_port_a);
		}

#ifdef DEBUG_TRACE_COM_HECTOR
		printf("\nEcriture data par Hector %x (dans portA)",data);
#endif
	}

	if ((offset & 0x3) == 0x1) /* Port B */
		state->m_hector_port_b = data;


	if ((offset & 0x3) == 0x2) /* Port C => depending cmd word */
	{
		if (!BIT(state->m_hector_port_cmd, 0))	 /* cmd -> Quartet inf en sortie ?*/
		{
			state->m_hector_port_c_l = data & 0x0f;
			// Utilizing bits port C : PC0 for the printer : strobe!
			if (BIT(state->m_hector_port_c_l  , 0))		// PC0 (bit X0)= true
			{
				/* Port A => to printer*/
				//printer_output(space->machine().device("printer"), state->m_hector_port_a);
			}
			// Utilizing bits port C : PC1 // PC2  for the communication with disc2
			if (!BIT(state->m_hector_port_c_l  , 1))		// PC1 (bit X1)= true
			{
				// Lecture effectuee => RAZ memoire donnee state->m_hector_disc2_data_write dispo
				state->m_hector_port_b = state->m_hector_disc2_data_write; // Mep sur port B si 2eme 74374 existant !
				state->m_hector_disc2_data_w_ready = 0x00;
				#ifdef DEBUG_TRACE_COM_HECTOR
					printf("\nEcriture port B vers state->m_hector_disc2_data_write suite a PC1");
				#endif
			}
			if (!BIT(state->m_hector_port_c_l, 2))		// PC2 (bit X2)= true
			{
				state->m_hector_disc2_data_read = state->m_hector_port_a; /* mise en place de l'info presente sur le port A */
				state->m_hector_disc2_data_r_ready = 0x08;		 /* memorisation de l'info */
				#ifdef DEBUG_TRACE_COM_HECTOR
					printf("\nEcriture port A pour state->m_hector_disc2_data_read suite a PC2");
				#endif
			}
		}
		if (!BIT(state->m_hector_port_cmd, 3))	 /* cmd -> Quartet sup en sortie ?*/
			state->m_hector_port_c_h = (data & 0xf0);
	}

	if ((offset & 0x3) == 0x3) /* Port commande */
	{
		state->m_hector_port_cmd = data;
	}
}
/* End of 8255 managing */


/*******************  Ecriture PIO specifique machine MX40 *******************/
WRITE8_HANDLER( hector_mx40_io_port_w)
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
/* Bank switching on several address */
	if ((offset &0x0ff) == 0x40) /* Port page 0*/
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE0);
	if ((offset &0x0ff) == 0x41) /* Port page 1*/
	{
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE1);
		state->m_hector_flag_80c=0;
	}
	if ((offset &0x0ff) == 0x44) /* Port page 2  => 42 pour MX80*/
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE2);
	if ((offset &0x0ff) == 0x49) /* Port screen resolution*/
		state->m_hector_flag_80c=0;/* No 80c in 40c !*/
}

/*******************  Ecriture PIO specifique machine MX80 *******************/
WRITE8_HANDLER( hector_mx80_io_port_w)
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	if ((offset &0x0ff) == 0x40) /* Port page 0*/
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE0);
	if ((offset &0x0ff) == 0x41) /* Port page 1*/
	{
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE1);
		state->m_hector_flag_80c=0;
	}
	if ((offset &0x0ff) == 0x42) /* Port page 2  => port different du MX40*/
		memory_set_bank(space->machine(), "bank2", HECTORMX_BANK_PAGE2);
	if ((offset &0x0ff) == 0x49) /* Port screen resolution*/
		state->m_hector_flag_80c=1;
}

/********************************************************************************
 sound managment
********************************************************************************/

static void Mise_A_Jour_Etat(running_machine &machine, int Adresse, int Value )
{
	hec2hrp_state *state = machine.driver_data<hec2hrp_state>();
/* Adjust value depending on I/O main CPU request*/
switch(Adresse )
{
	case 0x2000:
		/* Modification AU0 / AU8*/
		{	/* AU0*/
			state->m_AU[ 0] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU8 : 0*/
			state->m_AU[ 8] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2001:
		/* Modification AU1 / AU9*/
		{	/* AU1*/
			state->m_AU[ 1] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU9*/
			state->m_AU[ 9] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2002:
		/* Modification AU2 / AU10*/
		{	/* AU2*/
			state->m_AU[ 2] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU10*/
			state->m_AU[10] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2003:
		/* Modification AU3 / AU11*/
		{	/* AU3*/
			state->m_AU[ 3] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU11*/
			state->m_AU[11] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2800:
		/* Modification AU4 / AU12*/
		{	/* AU4*/
			state->m_AU[ 4] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU8*/
			state->m_AU[12] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2801:
		/* Modification AU5 / AU13*/
		{	/* AU5*/
			state->m_AU[ 5] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU13*/
			state->m_AU[13] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2802:
		{	/* Modification AU6 / AU14*/
			/* AU6*/
			state->m_AU[ 6] = ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU14*/
			state->m_AU[14] = ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x2803:
		/* Modification AU7 / AU15*/
		{	/* AU7*/
			state->m_AU[ 7] =  ((Value & 0x080 )==0) ? 0 : 1 ;
			/* AU15*/
			state->m_AU[15] =  ((Value & 0x040 )==0) ? 0 : 1 ;
			break;
		}
	case 0x3000:
		/* Mixer modification*/
		{
			state->m_ValMixer = (Value & 7) ;
			break;
		}
	default: break;
} /*switch*/
}


static void Init_Value_SN76477_Hector(running_machine &machine)
{
	hec2hrp_state *state = machine.driver_data<hec2hrp_state>();
	/* Remplissage des valeurs de resistance et capacite d'Hector*/

	/* Decay R*/
	state->m_Pin_Value[7][1] = RES_K(680.0); /*680K  */
	 state->m_Pin_Value[7][0] = RES_K(252.325); /* 142.325 (680 // 180KOhm)*/

	/* Capa A/D*/
	state->m_Pin_Value[8][0] = CAP_U(0.47); /* 0.47uf*/
	state->m_Pin_Value[8][1] = CAP_U(1.47);  /* 1.47*/

	/* ATTACK R*/
	state->m_Pin_Value[10][1]= RES_K(180.0);   /* 180*/
	state->m_Pin_Value[10][0]= RES_K(32.054); /* 32.054 (180 // 39 KOhm)*/

	/* Version 3 : Ajuste pour les frequences mesurees :
                // 4  0 SOUND 255 Hz => ajuste a l'oreille
                // 4  4 SOUND  65 Hz => ajuste a l'oreille
                // 4  8 SOUND  17 Hz =>  ajuste a l'oreille
                // 4 12 SOUND 4,3 Hz =>  ajuste a l'oreille*/
	/*   SLF C       Version 3*/
	state->m_Pin_Value[21][0]= CAP_U(0.1);  /*CAPU(0.1) */
	state->m_Pin_Value[21][1]= CAP_U(1.1);  /*1.1*/

	/*SLF R        Version 3*/
	state->m_Pin_Value[20][1]= RES_K(180);    //180 vu
	state->m_Pin_Value[20][0]= RES_K(37.268); //37.268 (47//180 KOhms)

	/* Capa VCO*/
	/* Version 3 : Ajust?? pour les frequences mesur??es :
            // 0 0  SOUND 5,5KHz => 5,1KHz
            // 0 16 SOUND 1,3KHz => 1,2KHz
            // 0 32 SOUND 580Hz  => 570Hz
            // 0 48 SOUND 132Hz  => 120Hz*/
	state->m_Pin_Value[17][0] = CAP_N(47.0) ;  /*47,0 mesure ok */
	state->m_Pin_Value[17][1] = CAP_N(580.0) ; /*580  mesure ok */
	/* R VCO   Version 3*/
	state->m_Pin_Value[18][1] = RES_K(1400.0   );/*1300 mesure ok    // au lieu de 1Mohm*/
	state->m_Pin_Value[18][0] = RES_K( 203.548 );/*223  mesure ok    // au lieu de 193.548 (1000 // 240KOhm)*/

	/* VCO Controle*/
	state->m_Pin_Value[16][0] = 0.0;  /* Volts  */
	state->m_Pin_Value[16][1] = 1.41; /* 2 =  10/15eme de 5V*/

	/* Pitch*/
	state->m_Pin_Value[19][0] = 0.0;   /*Volts */
	state->m_Pin_Value[19][1] = 1.41;

	state->m_Pin_Value[22][0] = 0; /* TOR */
	state->m_Pin_Value[22][1] = 1;

	/* R OneShot*/
	state->m_Pin_Value[24][1] = RES_K(100);
	 state->m_Pin_Value[24][0] = RES_K(1000);  /*RES_M(1) infini sur Hector car non connectee*/

	/* Capa OneShot*/
	state->m_Pin_Value[23][0] = 1.0;
	state->m_Pin_Value[23][1] = 0.0;  /* Valeur Bidon sur Hector car mise au 5Volts sans capa*/

	/* Enabled*/
	state->m_Pin_Value[9][0] = 0;
	state->m_Pin_Value[9][1] = 1;

	/* Volume*/
	state->m_Pin_Value[11][0] = 128; /* Rapport 50% et 100%  128*/
	state->m_Pin_Value[11][1] = 255; /*                      255*/

	/* Noise filter*/
	state->m_Pin_Value[6][0] = CAP_U(0.390);    /* 0.390*/
	state->m_Pin_Value[6][1] = CAP_U(08.60);    /* 0.48*/

	/* Valeur corrige par rapport au schema :*/
	state->m_Pin_Value[5][1] = RES_K(3.30 ) ;	/* 330Kohm*/
	state->m_Pin_Value[5][0] = RES_K(1.76 ) ;	/* 76 Kohm*/

	/* Noise pas commande par le bus audio !*/
	 /* Seule la valeur [0] est documentee !*/
	state->m_Pin_Value[4][0] = RES_K(47) ;		/* 47 K ohm*/
	state->m_Pin_Value[12][0] = RES_K(100);		/* 100K ohm*/
	state->m_Pin_Value[3][0] = 0 ;				/* NC*/

	/* Gestion du type d'enveloppe*/
	state->m_Pin_Value[ 1][0] = 0;
	state->m_Pin_Value[ 1][1] = 1;

	state->m_Pin_Value[28][0] = 0;
	state->m_Pin_Value[28][1] = 1;

	/* Initialisation a 0 des pin du SN*/
	state->m_AU[0]=0;
	state->m_AU[1]=0;
	state->m_AU[2]=0;
	state->m_AU[3]=0;
	state->m_AU[4]=0;
	state->m_AU[5]=0;
	state->m_AU[6]=0;
	state->m_AU[7]=0;
	state->m_AU[8]=0;
	state->m_AU[9]=0;
	state->m_AU[10]=0;
	state->m_AU[11]=0;
	state->m_AU[12]=0;
	state->m_AU[13]=0;
	state->m_AU[14]=0;
	state->m_AU[15]=0;
	state->m_ValMixer = 0;
}

static void Update_Sound(address_space *space, UINT8 data)
{
	hec2hrp_state *state = space->machine().driver_data<hec2hrp_state>();
	/* keep device*/
	device_t *sn76477 = space->machine().device("sn76477");

	/* MIXER*/
	sn76477_mixer_a_w(sn76477, ((state->m_ValMixer & 0x04)==4) ? 1 : 0);
	sn76477_mixer_b_w(sn76477, ((state->m_ValMixer & 0x01)==1) ? 1 : 0);
	sn76477_mixer_c_w(sn76477, ((state->m_ValMixer & 0x02)==2) ? 1 : 0);/* Revu selon mesure electronique sur HRX*/

	/* VCO oscillateur*/
	if (state->m_AU[12]==1)
		sn76477_vco_res_w(		sn76477, state->m_Pin_Value[18][state->m_AU[10]]/12.0); /* en non AU11*/
	else
		sn76477_vco_res_w(		sn76477, state->m_Pin_Value[18][state->m_AU[10]]); /* en non AU11*/

	sn76477_vco_cap_w(		sn76477, state->m_Pin_Value[17][state->m_AU[2 ]]);
	sn76477_pitch_voltage_w(sn76477, state->m_Pin_Value[19][state->m_AU[15]]);
	sn76477_vco_voltage_w(	sn76477, state->m_Pin_Value[16][state->m_AU[15]]);
	sn76477_vco_w(			sn76477, state->m_Pin_Value[22][state->m_AU[12]]); /* VCO Select Ext/SLF*/

	/* SLF*/
	sn76477_slf_res_w( sn76477, state->m_Pin_Value[20][state->m_AU[ 9]]);/*AU10*/
	sn76477_slf_cap_w( sn76477, state->m_Pin_Value[21][state->m_AU[1 ]]);

	/* One Shot*/
	sn76477_one_shot_res_w(sn76477, state->m_Pin_Value[24][     0]); /* NC*/
	sn76477_one_shot_cap_w(sn76477, state->m_Pin_Value[23][state->m_AU[13]]);

	/* Ampli value*/
	sn76477_amplitude_res_w(sn76477, state->m_Pin_Value[11][state->m_AU[5]]);

	/* Attack / Decay*/
	sn76477_attack_res_w(sn76477, state->m_Pin_Value[10][state->m_AU[ 8]]);
	sn76477_decay_res_w( sn76477, state->m_Pin_Value[7 ][state->m_AU[11]]);/*AU9*/
	sn76477_attack_decay_cap_w(sn76477, state->m_Pin_Value[8][state->m_AU[0]]);

	/* Filtre*/
	sn76477_noise_filter_res_w(sn76477, state->m_Pin_Value[5][state->m_AU[4]]);
	sn76477_noise_filter_cap_w(sn76477, state->m_Pin_Value[6][state->m_AU[3]]);

	/* Clock Extern Noise*/
	sn76477_noise_clock_res_w(sn76477, state->m_Pin_Value[4][0]);	/* fix*/
	sn76477_feedback_res_w(sn76477, state->m_Pin_Value[12][0]);		/*fix*/

	/*  Envelope*/
	sn76477_envelope_1_w(sn76477, state->m_Pin_Value[1 ][state->m_AU[6]]);
	sn76477_envelope_2_w(sn76477, state->m_Pin_Value[28][state->m_AU[7]]);

	/* En dernier on lance (ou pas !)*/
	sn76477_enable_w(sn76477, state->m_Pin_Value[9][state->m_AU[14]]);
}

const sn76477_interface hector_sn76477_interface =
{
	RES_K(47),		/*  4  noise_res*/
	RES_K(330),		/*  5  filter_res*/
	CAP_P(390),		/*  6  filter_cap*/
	RES_K(680),		/*  7  decay_res*/
	CAP_U(47),		/*  8  attack_decay_cap*/
	RES_K(180),		/* 10  attack_res*/
	RES_K(33),		/* 11  amplitude_res*/
	RES_K(100),		/* 12  feedback_res*/
	2,				/* 16  vco_voltage*/
	CAP_N(47) ,		/* 17  vco_cap*/
	RES_K(1000),	/* 18  vco_res*/
	2,				/* 19  pitch_voltage*/
	RES_K(180),		/* 20  slf_res*/
	CAP_U(0.1),		/* 21  slf_cap*/
	CAP_U(1.00001),	/* 23  oneshot_cap*/
	RES_K(10000)	/* 24  oneshot_res*/
};

void hector_reset(running_machine &machine, int hr, int with_D2 )
{
	hec2hrp_state *state = machine.driver_data<hec2hrp_state>();
	// Initialization Hector
	state->m_hector_flag_hr = hr;
	state->m_flag_clk = 0;
	state->m_write_cassette = 0;
	cputag_set_input_line(machine, "maincpu" , INPUT_LINE_RESET, PULSE_LINE);

	// Initialization Disc II
	if (with_D2==1)

	{
		cputag_set_input_line(machine, "disc2cpu", INPUT_LINE_RESET, PULSE_LINE);
		device_t *fdc = machine.device("upd765");
		cputag_set_input_line(machine, "disc2cpu", INPUT_LINE_RESET, PULSE_LINE);
		upd765_reset(fdc, 1);
		upd765_reset_w(fdc, 1);
	}
}

void hector_init(running_machine &machine)
{
	hec2hrp_state *state = machine.driver_data<hec2hrp_state>();
	state->m_pot0 = state->m_pot1 = 0x40;

	/* For Cassette synchro*/
	state->m_Cassette_timer = machine.scheduler().timer_alloc(FUNC(Callback_CK));
	state->m_Cassette_timer->adjust(attotime::from_msec(100), 0, attotime::from_usec(64));/* => real synchro scan speed for 15,624Khz*/

	/* Sound sn76477*/
	Init_Value_SN76477_Hector(machine);  /*init R/C value*/
}
