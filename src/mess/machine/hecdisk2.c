/*******************************************************/
/************ HECDISK2.C  in machine  ******************/
/*******************************************************/

/* Lecteur de disquette DISK II pour les machines :
        Hector HRX
        Hector MX40c
        Hector MX80c

		JJStacino  jj.stacino@aliceadsl.fr
		
	26/09/2010 : first sending with bug2 (the first "dir" command finih with a crash of the Z80 disk II proc)
*/
/* explication bug 1.
A Y E : Les interruptions pour venir en 6C4 se font à un rythme trop grand : des que le processeur
à envoyé la commande au uPD celui ci repond NMI.
sans laisser le temps au prog d'avancer !!!
Ensuite à chaque pas apres un IN j'ai une nouvelle interruption sans avoir termine la precedente. C'est
pourquoi en visu cela ne fait que de in en 6b4 sans jamais pouvoir aller en inc HL !!! Le positonnement de
HL se faisant apres le pop HL de x0687 - e ne se faisait pas !!!! 
(ff09 etait l'adresse du dernier octet de commande au uPD !!!
bug 2 : Revoir les interruptionse & NMI.
*/

#include "emu.h"

#include "sound/sn76477.h"   /* for sn sound*/
#include "sound/wave.h"      /* for K7 sound*/
#include "sound/discrete.h"  /* for 1 Bit sound*/
#include "machine/upd765.h"	/* for floppy disc controller */
#include "formats/basicdsk.h"
#include "cpu/z80/z80.h"

#include "includes/hec2hrp.h"

/* Disquette timer*/
static TIMER_CALLBACK( Callback_DMA_irq );
static emu_timer *DMA_timer;
static TIMER_CALLBACK( Callback_INT_irq );
static emu_timer *INT_timer;


/* Callback uPD request */
static WRITE_LINE_DEVICE_HANDLER( disk2_fdc_dma_irq );

UINT8 Disk2memory[0x10000];  /* Memory space for Disk II unit*/
UINT8 Mem_RNMI =0xff;
/* Buffer of the 74374 (IC1 and IC4) linked to the Hector port */
UINT8 disk2_data_r_ready=0x0; /* =ff when PC2 = true and data in read buffer (disk2_data_read) */
UINT8 disk2_data_w_ready=0x0; /* =ff when Disk 2 Port 40 had send a data in write buffer (disk2_data_write) */
UINT8 disk2_data_read=0;    /* Data send by Hector to Disk 2 when PC2=true */
UINT8 disk2_data_write=0;   /* Data send by Disk 2 to Hector when Write Port I/O 40 */
UINT8 disk2_RNMI = 0;		/* State of I/O 50 D5 = authorization for INT / NMI */
static int NMI_current_state=0;
static int INT_current_state=0;
static int Time_arq = 4000; 

/* Fonctionnement du uPD765 : 

               * ecriture du N° de comande suivi des octets necessaire a la commande (selon doc uPD) OUT ($61)
               * Lecture ou ecriture des data en dma :
                         Demande DMA uPD => Z80 : NMI,
                         Lecture  octet IN  ($70) par le Z80 => DRQ uPD ou
                         Ecriture octet OUT ($70) par le Z80 => DRQ uPD
               * En fin d'execution commande => INT.
               * ff12 = 12 lors de l'INT,
               * Sortie du programme d'attente => vers lecture compte rendu
               * Lecture des octets de compte rendu IN ($61)

*/
                         
FLOPPY_OPTIONS_START( hector_disk2 )
	FLOPPY_OPTION( disk2_h, "td0", "Disk II Micronique disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([00]))
FLOPPY_OPTIONS_END
/* Lorsque systemMX.td0 est traduit avec TD02IMD : 
IMD TD 1.5 5.25 LD MFM S-step, 1 sides 9/08/2009 20:10:17
HECTOR SYSTEM CP/M 2.2 Disk
*/

/* Hector Disk II uPD765 interface use interrupts and DMA! */
const upd765_interface disk2_upd765_interface =
{
	DEVCB_LINE(disk2_fdc_interrupt),
	DEVCB_LINE(disk2_fdc_dma_irq),
    NULL,  //	disk2_fdc_get_image,
	UPD765_RDY_PIN_NOT_CONNECTED,  //NOT_
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

// Source  : System.cfg
//Capacité 	Faces 	Densité Codage Pistes Secteurs 	Taille  	Lecteurs / Ordinateurs 
//	(Ko)											(octet)
//	200 		1 	DD 		MFM 	40		10 		512 		Micronique Hector 
//	800 		2 	DD 		MFM 	80 		10 		512 		Micronique Hector 
const floppy_config disk2_floppy_config =
{
	DEVCB_NULL,  // 
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,    //DSHD, for other 800Ko disk
	FLOPPY_OPTIONS_NAME(hector_disk2),
//	FLOPPY_OPTIONS_NAME(default),
	NULL 
};

WRITE8_HANDLER( hector_disk2_w )
{
//if ((offset >= 0x0fff)) // || (offset == 0x066) || (offset == 0x067) || (offset == 0x068))
	Disk2memory[offset]= data;
	
/* Autorisation d'ecriture uniquement dans les zones autorisees
if (offset == 0x066)
		Disk2memory[offset]= data;
if (offset == 0x067)
		Disk2memory[offset]= data;
if (offset == 0x068)
		Disk2memory[offset]= data;
if (offset == 0x038)
		Disk2memory[offset]= data;
if (offset == 0x039)
		Disk2memory[offset]= data;
if (offset == 0x03A)
		Disk2memory[offset]= data;*/
}

READ8_HANDLER( hector_disk2_r )
{
UINT8 data =0;
data = Disk2memory[offset];
//if (offset == 0xff6b)
//   data = 0x0ff; // force mode verbose on Hector !
return data;
}

void valid_interrupt( running_machine *machine)
{
/* Called at each rising state of INT / NMI and RNMI ! */

/* Take NMI / INT  only if RNMI ok*/

/* Checking for NMI interrupt */
if ((disk2_RNMI ==0x00) && (NMI_current_state!=0))
    {    
		cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_NMI, CLEAR_LINE); // NMI...
		timer_adjust_oneshot(DMA_timer, ATTOTIME_IN_NSEC(Time_arq), 0 );
        Time_arq = 4000; //  6900us for next step !
        NMI_current_state=0; /* clear the current request*/
	}

/* Checking for INT interrupt */
if ((disk2_RNMI ==0x00) && (INT_current_state!=0))
    {
		cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_IRQ0, CLEAR_LINE); //INT...
		timer_adjust_oneshot(INT_timer, ATTOTIME_IN_MSEC(400), 0 ); //2010  =  8000
       INT_current_state=0; /* clear the current request*/
	   //printf("\nLancement interruption !");
    }
 
}            

void Init_Timer_DiskII( running_machine *machine)
{
       DMA_timer = timer_alloc(machine, Callback_DMA_irq, 0);
       INT_timer = timer_alloc(machine, Callback_INT_irq, 0);
}

static TIMER_CALLBACK( Callback_DMA_irq )
{
/* To generate the NMI signal (late) when uPD DMA request*/
cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_NMI, ASSERT_LINE);  //NMI...

}
static TIMER_CALLBACK( Callback_INT_irq )
{
/* To generate the INT signal (late) when uPD INT request*/
cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_IRQ0, ASSERT_LINE);  //INT...
}
     
/* upd765 INT is connected to interrupt of Z80 */
static WRITE_LINE_DEVICE_HANDLER( disk2_fdc_interrupt )
{
    INT_current_state = state;
    valid_interrupt(device->machine);
}
/* upd765 DRQ is connected to NMI of Z80 within a RNMI hardware authorization*/
static WRITE_LINE_DEVICE_HANDLER( disk2_fdc_dma_irq )
{
     NMI_current_state = state;                  
     valid_interrupt(device->machine);
}
/////////////////////////////////////
// Port handling of the Disk II unit
/////////////////////////////////////
READ8_HANDLER( disk2_io30_port_r)
{
 return disk2_data_r_ready;
}
WRITE8_HANDLER( disk2_io30_port_w)
{
}
READ8_HANDLER( disk2_io40_port_r)
{
UINT8 data;

data = disk2_data_read;
disk2_data_r_ready = 0x00; /* Raz memoire info read dispo*/
return data;   /* send thez data !*/
}
WRITE8_HANDLER( disk2_io40_port_w)
{
/* Write a data */
disk2_data_write = data;	/* Memorisation donnee*/
disk2_data_w_ready = 0xff;  /* Memorisation donnee dispo*/
}

READ8_HANDLER( disk2_io50_port_r)
{
return disk2_data_w_ready;
}
WRITE8_HANDLER( disk2_io50_port_w)
{
/* FDC Motor Control - Bit 0/1 defines the state of the FDD motor:
                * "1" the FDD motor will be active.
                * "0" the FDD motor will be in-active.*/

running_device *fdc = space->machine->device("upd765");

floppy_mon_w(floppy_get_device(space->machine, 0), !BIT(data, 0));
floppy_mon_w(floppy_get_device(space->machine, 1), !BIT(data, 1));
floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,1);
floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), 1,1);

upd765_tc_w(fdc, ASSERT_LINE);

/* Ecriture bit TC uPD765 sur D4 du port 50 */
if (BIT(data, 4))
   upd765_tc_w(fdc, 1);
else
   upd765_tc_w(fdc, 0);

/* Authorization interrupt and NMI */
if BIT(data, 5)
	disk2_RNMI=0xff;
else
	disk2_RNMI=0x00;

/* if the programm give the authorization => try to interrupt and NMI ! */
//if 	(disk2_RNMI==0xff)
//  valid_interrupt(space->machine);
  
}


READ8_HANDLER( disk2_io60_port_r)
{
/* Lecture du status uPD*/
UINT8 data=0;
 running_device *fdc = space->machine->device("upd765");
 
data = upd765_status_r(fdc, 0);

return data;  
}
READ8_HANDLER( disk2_io61_port_r)
{
/* Lecture d'une data uPD*/
UINT8 data=0;
 running_device *fdc = space->machine->device("upd765");
 
data=upd765_data_r(fdc, 0); /* when pin A0 =1 */

return data;  
}
WRITE8_HANDLER( disk2_io60_port_w)
{
/* Ecriture du status uPD ??? impossible ! */
 running_device *fdc = space->machine->device("upd765");

upd765_data_w(fdc, 0,data);  //upd765_data_w(device, offset, data);

}
WRITE8_HANDLER( disk2_io61_port_w)
{
/* Ecriture d'une data uPD*/
 running_device *fdc = space->machine->device("upd765");

upd765_data_w(fdc, 0,data);  //upd765_data_w(device, offset, data);

}
READ8_HANDLER( disk2_io70_port_r)
{
/* Read DMA Data of FDC */
UINT8 data=0;
//UINT16 hlreg;/*Used to debug*/

running_device *fdc = space->machine->device("upd765");
data=upd765_dack_r(fdc, 1); /* when pin A0 =1*/
//hlreg = cpu_get_reg(space->machine->device("disk2cpu"), Z80_HL); // voir H   L

//printf("%c",hlreg);
return data;  
}
WRITE8_HANDLER( disk2_io70_port_w)
{
running_device *fdc = space->machine->device("upd765");
/* Write DMA Data on FDC */
upd765_dack_w(fdc, 0,data);
}
