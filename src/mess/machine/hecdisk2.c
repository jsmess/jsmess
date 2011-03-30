/*******************************************************/
/************ HECDISK2.C  in machine  ******************/
/*******************************************************/

/* Lecteur de disquette DISK II pour les machines :
        Hector HRX
        Hector MX40c
        Hector MX80c

        JJStacino  jj.stacino@aliceadsl.fr

    15/02/2010 : Start of the disc2 project! JJStacino
    26/09/2010 : first sending with bug2 (the first "dir" command finih with a crash of the Z80 disc II proc) JJStacino
    01/11/2010 : first time ending boot sequence, probleme on the CP/M lauch JJStacino
    20/11/2010 : synchronization between uPD765 and Z80 are now OK, CP/M runnig! JJStacino
    28/11/2010 : Found at Bratislava that the disk writing with TRANS X: is NOT WORKING (the exchange Hector=>Disc2 ok)
*/

#include "emu.h"

#include "sound/sn76477.h"		/* for sn sound*/
#include "sound/wave.h"			/* for K7 sound*/
#include "sound/discrete.h"		/* for 1 Bit sound*/
#include "machine/upd765.h"		/* for floppy disc controller */
#include "formats/basicdsk.h"
#include "cpu/z80/z80.h"

#include "includes/hec2hrp.h"

#ifndef hector_trace
// hector_trace = 1  // Useful to trace the exchange z80<=>uPD765
#endif

/* Disquette timer*/
static TIMER_CALLBACK( Callback_DMA_irq );
static emu_timer *DMA_timer;
static TIMER_CALLBACK( Callback_INT_irq );
static emu_timer *INT_timer;

/* Callback uPD request */
//UPD765_DMA_REQUEST( hector_disc2_fdc_dma_irq );
static WRITE_LINE_DEVICE_HANDLER( disc2_fdc_interrupt );
static WRITE_LINE_DEVICE_HANDLER( hector_disc2_fdc_dma_irq );

/* Buffer of the 74374 (IC1 and IC4) linked to the Hector port */
UINT8 hector_disc2_data_r_ready;/* =ff when PC2 = true and data in read buffer (hector_disc2_data_read) */
UINT8 hector_disc2_data_w_ready;/* =ff when Disc 2 Port 40 had send a data in write buffer (hector_disc2_data_write) */
UINT8 hector_disc2_data_read;	/* Data send by Hector to Disc 2 when PC2=true */
UINT8 hector_disc2_data_write;	/* Data send by Disc 2 to Hector when Write Port I/O 40 */
UINT8 hector_disc2_RNMI;		/* State of I/O 50 D5 = authorization for INT / NMI */
static int NMI_current_state;

// useful for patch uPD765 result----start
static int hector_cmd_0, hector_cmd_1, hector_cmd_2, hector_cmd_3, hector_cmd_4 ;
static int hector_cmd_5, hector_cmd_6, hector_cmd_7, hector_cmd_8, hector_cmd_9 ;
static int hector_nb_cde = 0;
static int hector_flag_result = 0;
#ifdef hector_trace
static int print =0;
#endif
// useful for patch uPD765 result----end

/* How uPD765 works:
        * First we send at uPD the string of command (p.e. 9 bytes for read starting by 0x46) on port 60h
                between each byte, check the authorization of the uPD by reading the status register
        * When the command is finish, the data arrive with DMA interrupt, then:
                If read: in port 70 to retrieve the data,
                If write: in port 70 send the data
        * When all data had been send the uPD launch an INT
        * The Z80 Disc2 writes in FF12 a flag
        * if the flag is set, end of DMA function,
        * At this point the Z80 can read the RESULT in port 61h
*/

/*****************************************************************************/
/******  Management of the floppy images 200Ko and 800Ko *********************/
/*****************************************************************************/
/* For the 200Ko disk :
        512 bytes per sectors,
        10  sector per track,
        From sector =0 to sector 9,
        40  tracks,
        1 Head
    This format can be extract from a real disc with anadisk (*.IMG format rename in *.HE2).
*/
static FLOPPY_IDENTIFY(hector_disc2_dsk200_identify)
{
	*vote = (floppy_image_size(floppy) == (1*40*10*512)) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(hector_disc2_dsk200_construct)
{
	struct basicdsk_geometry geometry;
	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 1;
	geometry.first_sector_id = 0;
	geometry.sector_length = 512;
	geometry.tracks = 40;
	geometry.sectors = 10;
	return basicdsk_construct(floppy, &geometry);
}
/* For the 720Ko disk :
        512 bytes per sectors,
        9  sector per track,
        From sector =0 to sector 8,
        80  tracks,
        2 Head
    This format can be extract from a real disc with anadisk (*.IMG format rename in *.HE7).
*/
static FLOPPY_IDENTIFY(hector_disc2_dsk720_identify)
{
	*vote = (floppy_image_size(floppy) == (2*80*9*512)) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(hector_disc2_dsk720_construct)
{
	struct basicdsk_geometry geometry;
	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 2;
	geometry.first_sector_id = 0;
	geometry.sector_length = 512;
	geometry.tracks = 80;
	geometry.sectors = 9;
	return basicdsk_construct(floppy, &geometry);

}/* For the 800Ko disk :
        512 bytes per sectors,
        10  sector per track,
        From sector =0 to sector 9,
        80  tracks
        2 Heads
    This format can be extract from a real disk with anadisk (*.IMG format rename in *.HE2).
*/

static FLOPPY_IDENTIFY(hector_disc2_dsk800_identify)
{
	*vote = (floppy_image_size(floppy) == (2*80*10*512)) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(hector_disc2_dsk800_construct)
{
	struct basicdsk_geometry geometry;
	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 2;
	geometry.first_sector_id = 0;
	geometry.sector_length = 512;
	geometry.tracks = 80;
	geometry.sectors = 10;
	return basicdsk_construct(floppy, &geometry);
}

FLOPPY_OPTIONS_START( hector_disc2 )
	FLOPPY_OPTION( hector_dsk, "HE2", "hector disc2 floppy disk image 200K", hector_disc2_dsk200_identify, hector_disc2_dsk200_construct, NULL)
	FLOPPY_OPTION( hector_dsk, "HE7", "hector disc2 floppy disk image 720K", hector_disc2_dsk720_identify, hector_disc2_dsk720_construct, NULL)
	FLOPPY_OPTION( hector_dsk, "HE8", "hector disc2 floppy disk image 800K", hector_disc2_dsk800_identify, hector_disc2_dsk800_construct, NULL)
FLOPPY_OPTIONS_END

// Define the hardware of the disk
 const floppy_config hector_disc2_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(hector_disc2),
	NULL
};

/*****************************************************************************/
/******  Management of the uPD765 for interface with floppy images************/
/*****************************************************************************/
/* Hector Disc II uPD765 interface use interrupts and DMA! */
const upd765_interface hector_disc2_upd765_interface =
{
	DEVCB_LINE(disc2_fdc_interrupt),
	DEVCB_LINE(hector_disc2_fdc_dma_irq),
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

/*****************************************************************************/
/****  Management of the interrupts (NMI and INT)between uPD765 and Z80 ******/
/*****************************************************************************/
static void valid_interrupt( running_machine &machine)
{
/* Called at each rising state of NMI or RNMI ! */

/* Take NMI only if RNMI ok*/
if ((hector_disc2_RNMI==1) &&  (NMI_current_state==1))
	{
		cputag_set_input_line(machine, "disc2cpu", INPUT_LINE_NMI, CLEAR_LINE); // Clear NMI...
		DMA_timer->adjust(attotime::from_usec(6) );//a little time to let the Z80 terminate he's previous job !
		NMI_current_state=0;
	}
}

void hector_disc2_init( running_machine &machine)
{
	DMA_timer = machine.scheduler().timer_alloc(FUNC(Callback_DMA_irq));
	INT_timer = machine.scheduler().timer_alloc(FUNC(Callback_INT_irq));
}

static TIMER_CALLBACK( Callback_DMA_irq )
{
	/* To generate the NMI signal (late) when uPD DMA request*/
	cputag_set_input_line(machine, "disc2cpu", INPUT_LINE_NMI, ASSERT_LINE);  //NMI...
	hector_nb_cde =0; // clear the cde lenght
}

static TIMER_CALLBACK( Callback_INT_irq )
{
	/* To generate the INT signal (late) when uPD DMA request*/
	cputag_set_input_line(/*device->*/machine, "disc2cpu", INPUT_LINE_IRQ0, ASSERT_LINE);
}

/* upd765 INT is connected to interrupt of Z80 within a RNMI hardware authorization*/
static WRITE_LINE_DEVICE_HANDLER( disc2_fdc_interrupt )
{
	cputag_set_input_line(device->machine(), "disc2cpu", INPUT_LINE_IRQ0, CLEAR_LINE);
	if (state)
		INT_timer->adjust(attotime::from_usec(500) );//a little time to let the Z80 terminate he's previous job !
}

static WRITE_LINE_DEVICE_HANDLER( hector_disc2_fdc_dma_irq )
{
	/* upd765 DRQ is connected to NMI of Z80 within a RNMI hardware authorization*/
	/* Here the most difficult on this machine :
    The DMA request come with the uPD765 "soft" imediately,
    against the true hard uPD765. In the real life, the uPD had
    to seach for the sector before !
    So, we had to memorize the signal (the DMA is a pulse)
    until disc2's Z80 is ready to take the NMI interupt (when
    he had set the RNMI authorization) !      */

    if (state==1)
		NMI_current_state = state;
	valid_interrupt(device->machine());
}

// RESET the disc2 Unit !
void hector_disc2_reset(running_machine &machine)
{
	// Initialization Disc2 unit
	cputag_set_input_line(machine, "disc2cpu" , INPUT_LINE_RESET, PULSE_LINE);
	//switch ON and OFF the reset line uPD
	upd765_reset_w(machine.device("upd765"), 1);
	upd765_reset_w(machine.device("upd765"), 0);
	// Select ROM memory to cold restart
	memory_set_bank(machine, "bank3", DISCII_BANK_ROM);

	// Clear the Hardware's buffers
	hector_disc2_data_r_ready=0x0;	/* =ff when PC2 = true and data in read buffer (hector_disc2_data_read) */
	hector_disc2_data_w_ready=0x0;	/* =ff when Disc 2 Port 40 had send a data in write buffer (hector_disc2_data_write) */
	hector_disc2_data_read=0;		/* Data send by Hector to Disc 2 when PC2=true */
	hector_disc2_data_write=0;		/* Data send by Disc 2 to Hector when Write Port I/O 40 */
	hector_disc2_RNMI = 0;			/* State of I/O 50 D5 = authorization for INT / NMI */
	NMI_current_state=0;			/* Clear the DMA active request */
}

/*****************************************************************************/
/********************  Port handling of the Z80 Disc II unit *****************/
/*****************************************************************************/
READ8_HANDLER( hector_disc2_io00_port_r)
{
	/* Switch Disc 2 to RAM to let full RAM acces */
	memory_set_bank(space->machine(), "bank3", DISCII_BANK_RAM);
	return 0;
}
WRITE8_HANDLER( hector_disc2_io00_port_w)
{
	/* Switch Disc 2 to RAM to let full RAM acces */
	memory_set_bank(space->machine(), "bank3", DISCII_BANK_RAM);
}
READ8_HANDLER( hector_disc2_io20_port_r)
{
	// You can implemente the 8251 chip communication here !
	return 0;
}
WRITE8_HANDLER( hector_disc2_io20_port_w)
{
	// You can implemente the 8251 chip communication here !
}
READ8_HANDLER( hector_disc2_io30_port_r)
{
	return hector_disc2_data_r_ready;
}
WRITE8_HANDLER( hector_disc2_io30_port_w)
{
	// Nothing here !
}

READ8_HANDLER( hector_disc2_io40_port_r)
{
	/* Read data send by Hector, by Disc2*/
	hector_disc2_data_r_ready = 0x00;	/* Clear memory info read ready*/
	return hector_disc2_data_read;		/* send the data !*/
}

WRITE8_HANDLER( hector_disc2_io40_port_w)	/* Write data send by Disc2, to Hector*/
{
	hector_disc2_data_write = data;		/* Memorization data*/
	hector_disc2_data_w_ready = 0x80;	/* Memorization data write ready in D7*/
}

READ8_HANDLER( hector_disc2_io50_port_r)	/*Read memory info write ready*/
{
	return hector_disc2_data_w_ready;
}

WRITE8_HANDLER( hector_disc2_io50_port_w) /* I/O Port to the stuff of Disc2*/
{

	device_t *fdc = space->machine().device("upd765");

	/* FDC Motor Control - Bit 0/1 defines the state of the FDD 0/1 motor */
	floppy_mon_w(floppy_get_device(space->machine(), 0), BIT(data, 0));	// Moteur floppy A:
	floppy_mon_w(floppy_get_device(space->machine(), 1), BIT(data, 1));	// Moteur floppy B:
	floppy_drive_set_ready_state(floppy_get_device(space->machine(), 0), FLOPPY_DRIVE_READY,!BIT(data, 0));
	floppy_drive_set_ready_state(floppy_get_device(space->machine(), 1), FLOPPY_DRIVE_READY,!BIT(data, 1));

	/* Write bit TC uPD765 on D4 of port I/O 50 */
	upd765_tc_w(fdc, BIT(data, 4));  // Seems not used...


	/* Authorization interrupt and NMI with RNMI signal*/
	hector_disc2_RNMI = BIT(data, 5);

	/* if RNMI is OK, try to lauch an NMI*/
	if (hector_disc2_RNMI)
		valid_interrupt(space->machine());
}

//Here we must take the exchange with uPD against AM_DEVREADWRITE
// Because we had to add D6 = 1 when write is done with 0x28 in ST0 back

//  AM_RANGE(0x061,0x061) AM_DEVREADWRITE("upd765",upd765_data_r,upd765_data_w)
READ8_HANDLER( hector_disc2_io61_port_r)
{
	UINT8 data;
	device_t *fdc = space->machine().device("upd765");
	data = upd765_data_r(fdc,0); //Get the result

// if ST0 == 0x28 (drive A:) or 0x29 (drive B:) => add 0x40
// and correct the ST1 and ST2 (patch)
	if ((hector_flag_result == 3) & ((data==0x28) | (data==0x29)) ) // are we in the problem?
	{
		data=data + 0x40;
		hector_flag_result--;
	}
	// Nothing to do in over case!
	if (hector_flag_result == 3)
		hector_flag_result = 0;

	if ((hector_flag_result == 2) & (data==0x00) )
	{
		data=/*data +*/ 0x04;
		hector_flag_result--;
	}
	if ((hector_flag_result == 1) & (data==0x00) )
	{
		data=/*data + */0x10;
		hector_flag_result=0; // End !
	}
	#ifdef hector_trace
	if (print==1)
		printf(" _%x",data);
	#endif
	hector_nb_cde =0; // clear the cde lenght
	return data;
}
WRITE8_HANDLER( hector_disc2_io61_port_w)
{
/* Data useful to patch the RESULT in case of write command */
hector_cmd_9=hector_cmd_8;  //hector_cmd_8 = Cde number when hector_nb_cde = 9
hector_cmd_8=hector_cmd_7;  //hector_cmd_7 = Drive
hector_cmd_7=hector_cmd_6;  //hector_cmd_6 = C
hector_cmd_6=hector_cmd_5;  //hector_cmd_5 = H
hector_cmd_5=hector_cmd_4;  //hector_cmd_4 = R
hector_cmd_4=hector_cmd_3;  //hector_cmd_3 = N
hector_cmd_3=hector_cmd_2;  //hector_cmd_2 = EOT
hector_cmd_2=hector_cmd_1;  //hector_cmd_1 = GPL
hector_cmd_1=hector_cmd_0;  //hector_cmd_0 = DTL
hector_cmd_0 = data;
// Increase the lenght cde!
hector_nb_cde++;

// check if current commande is write cmde.
if (((hector_cmd_8 & 0x1f)== 0x05)  & (hector_nb_cde==9) ) /*Detect wrtie commande*/
	hector_flag_result = 3; // here we are!
#ifdef hector_trace
if (hector_nb_cde==6 ) /*Detect 1 octet command*/
{
	printf("\n commande = %x, %x, %x, %x, %x, %x Result = ", hector_cmd_5, hector_cmd_4, hector_cmd_3, hector_cmd_2, hector_cmd_1, data );
	print=1;
}
else
	print=0;
#endif

device_t *fdc = space->machine().device("upd765");
upd765_data_w(fdc,0, data);
}

//  AM_RANGE(0x070,0x07f) AM_DEVREADWRITE("upd765",upd765_dack_r,upd765_dack_w)
READ8_HANDLER( hector_disc2_io70_port_r) // Gestion du DMA
{
	UINT8 data;
	device_t *fdc = space->machine().device("upd765");
	data = upd765_dack_r(fdc,0);
	return data;
}
WRITE8_HANDLER( hector_disc2_io70_port_w)
{
	device_t *fdc = space->machine().device("upd765");
	upd765_dack_w(fdc,0, data);
}
