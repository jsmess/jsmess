/***************************************************************************

  machine\dgn_beta.c (machine.c)

	Moved out of dragon.c, 2005-05-05, P.Harvey-Smith.
	
	I decided to move this out of the main Dragon/CoCo source files, as 
	the Beta is so radically different from the other Dragon machines that 
	this made more sense (to me at least).

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  References:
	Disassembly of Dragon Beta ROM, examination of only (known) surviving board.
	
  TODO:
	Pretty much everything !
	
	Display working with 6845 taking data from rom.
	
  2005-05-10
	Memory banking seems to be working, as documented in code comments below.

  2005-05-31
	CPU#2 now executes code correctly to do transfers from WD2797.

  2005-06-03
  
	When fed a standard OS-9 boot disk it reads in the boot file and attempts
	to start it, not being able to find init, it fails. Hopefully I will
	soon have an image of a Beta boot disk.
	
  2005-11-29
  
	Major track tracing excersise on scans of bare beta board, reveal where a 
	whole bunch of the PIA lines go, especially the IRQs, most of them go back
	to the IRQ line on the main CPU.

  2005-12-07
  
	First booted to OS9 prompt, did not execute startup scripts.

  2005-12-08
	
	Fixed density setting on WD2797, so density of read data is now
	correctlty set as required by OS-9. This was the reason startup 
	script was not being executed as Beta disks have a single denisty 
	boot track, however the rest of the disk is double density.
	Booted completely to OS-9, including running startup script.
	
  2006-09-27
  
	Clean up of IRQ/FIRQ handling code allows correct booting again.
	
***************************************************************************/

#include <math.h>
#include <assert.h>

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/coco.h"
#include "includes/dgn_beta.h"
#include "includes/6551.h"
#include "image.h"
#include "machine/wd17xx.h"
#include "includes/crtc6845.h"

static UINT8 *system_rom;

/* Only log errors if debugging ! */
#ifdef MAME_DEBUG

//#define LOG_BANK_UPDATE	1
//#define LOG_DEFAULT_TASK	1
//#define LOG_PAGE_WRITE	1
//#define LOG_HALT	1
//#define LOG_TASK	1
//#define LOG_KEYBOARD	1
//#define LOG_VIDEO	1
#define LOG_DISK	0
//#define LOG_INTS	1
#endif

#ifdef MAME_DEBUG
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#endif /* MAME_DEBUG */

#ifdef MAME_DEBUG
static offs_t dgnbeta_dasm_override(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);
static void ToggleDatLog(int ref, int params, const char *param[]);
static void DumpKeys(int ref, int params, const char *param[]);

static int LogDatWrites;
#endif /* MAME_DEBUG */

static READ8_HANDLER(d_pia0_pa_r);
static WRITE8_HANDLER(d_pia0_pa_w);
static READ8_HANDLER(d_pia0_pb_r);
static WRITE8_HANDLER(d_pia0_pb_w);
static WRITE8_HANDLER(d_pia0_cb2_w);
static void d_pia0_irq_a(int state);
static void d_pia0_irq_b(int state);
static READ8_HANDLER(d_pia1_pa_r);
static WRITE8_HANDLER(d_pia1_pa_w);
static READ8_HANDLER(d_pia1_pb_r);
static WRITE8_HANDLER(d_pia1_pb_w);
static void d_pia1_irq_a(int state);
static void d_pia1_irq_b(int state);
static READ8_HANDLER(d_pia2_pa_r);
static WRITE8_HANDLER(d_pia2_pa_w);
static READ8_HANDLER(d_pia2_pb_r);
static WRITE8_HANDLER(d_pia2_pb_w);
static void d_pia2_irq_a(int state);
static void d_pia2_irq_b(int state);

static void cpu0_recalc_irq(int state);
static void cpu0_recalc_firq(int state);

static void cpu1_recalc_firq(int state);

static	int Keyboard[NoKeyrows];		/* Keyboard bit array */
static int RowShifter;				/* shift register to select row */
static int Keyrow;				/* Keyboard row being shifted out */
static int d_pia0_pb_last;			/* Last byte output to pia0 port b */
static int d_pia0_cb2_last;			/* Last state of CB2 */

static int KInDat_next;			/* Next data bit to input */
static int KAny_next;				/* Next value for KAny */
static int d_pia1_pa_last;
static int d_pia1_pb_last;
static int DMA_NMI_LAST;
//static int DMA_NMI;				/* DMA cpu has received an NMI */

#define INVALID_KEYROW	-1			/* no ketrow selected */
#define NO_KEY_PRESSED	0x7F			/* retrurned by hardware if no key pressed */

int SelectedKeyrow(int	Rows);

int dgnbeta_font=0;

static const pia6821_interface dgnbeta_pia_intf[] =
{
	/* PIA 0 at $FC20-$FC23 I46 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 	d_pia0_pa_r, d_pia0_pb_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ 	d_pia0_pa_w, d_pia0_pb_w, 0, d_pia0_cb2_w,
		/*irqs	 : A/B		   */ 	d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 at $FC24-$FC27 I63 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, 0,0,
		/*irqs	 : A/B		   */ d_pia1_irq_a, d_pia1_irq_b
	},

	/* PIA 2 at FCC0-FCC3 I28 */
	/* This seems to control the RAM paging system, and have the DRQ */
	/* from the WD2797 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia2_pa_r,d_pia2_pb_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia2_pa_w,d_pia2_pb_w, 0,0,
		/*irqs	 : A/B	   	   */ d_pia2_irq_a, d_pia2_irq_b
	}
};

// Info for bank switcher
struct bank_info_entry
{
	write8_handler handler;	// Pointer to write handler
	offs_t start;		// Offset of start of block
	offs_t end;		// offset of end of block
};

static const struct bank_info_entry bank_info[] =
{
	{ dgnbeta_ram_b0_w, 0x0000, 0x0fff },
	{ dgnbeta_ram_b1_w, 0x1000, 0x1fff },
	{ dgnbeta_ram_b2_w, 0x2000, 0x2fff },
	{ dgnbeta_ram_b3_w, 0x3000, 0x3fff },
	{ dgnbeta_ram_b4_w, 0x4000, 0x4fff },
	{ dgnbeta_ram_b5_w, 0x5000, 0x5fff },
	{ dgnbeta_ram_b6_w, 0x6000, 0x6fff },
	{ dgnbeta_ram_b7_w, 0x7000, 0x7fff },
	{ dgnbeta_ram_b8_w, 0x8000, 0x8fff },
	{ dgnbeta_ram_b9_w, 0x9000, 0x9fff },
	{ dgnbeta_ram_bA_w, 0xA000, 0xAfff },
	{ dgnbeta_ram_bB_w, 0xB000, 0xBfff },
	{ dgnbeta_ram_bC_w, 0xC000, 0xCfff },
	{ dgnbeta_ram_bD_w, 0xD000, 0xDfff },
	{ dgnbeta_ram_bE_w, 0xE000, 0xEfff },
	{ dgnbeta_ram_bF_w, 0xF000, 0xFBff },
	{ dgnbeta_ram_bG_w, 0xFF00, 0xFfff }
};

struct PageReg 
{
	int	value;			/* Value of the page register */
	UINT8	*memory;		/* The memory it actually points to */
};

static int 	TaskReg;			/* Task register, for current task */
static int	PIATaskReg;			/* Set to what PIA is set to, may not be same as TaskReg */
static int	EnableMapRegs;			/* Should we use map registers, or just map normal */
static struct PageReg	PageRegs[MaxTasks+1][MaxPage+1];	/* 16 sets of 16 page regs, allowing for 16 seperate contexts */
						/* Task 17 is power on default, when banking not enabled */

int IsIOPage(int	Page)
{
	if ((Page==IOPage) || (Page==IOPage+1))
		return 1;
	else
		return 0;
}

//
// Memory pager, maps machine's 1Mb total address space into the 64K addressable by the 6809.
// This is in some way similar to the system used on the CoCo 3, except the beta uses 4K 
// pages (instead of 8K on the CoCo), and has 16 task registers instead of the 2 on the CoCo.
//
// Each block of 16 page registers (1 for each task), is paged in at $FE00-$FE0F, the bottom 
// 4 bits of the PIA register at $FCC0 seem to contain which task is active.
// bit 6 of the same port seems to enable the memory paging
//
// For the purpose of this driver any block that is not ram, and is not a known ROM block,
// is mapped to the first page of the boot rom, I do not know what happens in the real
// hardware, however this does allow the boot rom to correctly size the RAM. 
// this should probably be considdered a hack !
//

static void UpdateBanks(int first, int last)
{
	int		Page;
	UINT8 		*readbank;
	write8_handler 	writebank;
	int		bank_start;
	int		bank_end;
	int		MapPage;
#ifdef LOG_BANK_UPDATE
	logerror("\n\nUpdating banks %d to %d at PC=$%X\n",first,last,activecpu_get_pc());
#endif
	for(Page=first;Page<=last;Page++)
	{
		bank_start	= bank_info[Page].start;
		bank_end	= bank_info[Page].end;
		
		if (!IsIOPage(Page))
			MapPage	= PageRegs[TaskReg][Page].value;
		else
			MapPage = PageRegs[TaskReg][15].value;

		//
		// Map block, $00-$BF are ram, $FC-$FF are Boot ROM
		//
		if ((MapPage*4) < ((mess_ram_size / 1024)-1))		// Block is ram
		{
			if (!IsIOPage(Page)) 		
			{
				readbank = &mess_ram[MapPage*RamPageSize];
#ifdef MAME_DEBUG
				if(LogDatWrites)
					debug_console_printf("Mapping page %X, pageno=%X, mess_ram[%X]\n",Page,MapPage,(MapPage*RamPageSize));
#endif
			}
			else
			{
				readbank = &mess_ram[(MapPage*RamPageSize)-256];
				logerror("Error RAM in IO PAGE !\n");
			}
			writebank=bank_info[Page].handler;
		}
		else					// Block is rom, or undefined
		{
			if (MapPage>0xfB)
			{
				if (Page!=IOPage+1)
					readbank=&system_rom[(MapPage-0xFC)*0x1000];		
				else
					readbank=&system_rom[0x3F00];
			}
			else
				readbank=system_rom;
			
			writebank=MWA8_ROM;
		}

		PageRegs[TaskReg][Page].memory=readbank;
		memory_set_bankptr(Page+1,readbank);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);
		memory_install_write8_handler(1, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);

#ifdef LOG_BANK_UPDATE
		logerror("UpdateBanks:MapPage=$%02X readbank=$%X\n",MapPage,(int)readbank);
		logerror("PageRegsSet Task=%X Page=%x\n",TaskReg,Page);
		logerror("memory_set_bankptr(%X)\n",Page+1);
		logerror("memory_install_write8_handler CPU=0\n");
		logerror("memory_install_write8_handler CPU=1\n");
#endif
	}
}

//
static void SetDefaultTask(void)
{
	int		Idx;

#ifdef LOG_DEFAULT_TASK	
	logerror("SetDefaultTask()\n");
	debug_console_printf("Set Default task\n");
#endif

	TaskReg=NoPagingTask;
	
	/* Reset ram pages */
	for(Idx=0;Idx<ROMPage-1;Idx++)
	{
		PageRegs[TaskReg][Idx].value=NoMemPageValue;
	}		

	/* Reset RAM Page */
	PageRegs[TaskReg][RAMPage].value=RAMPageValue;
	
	/* Reset Video mem page */
	PageRegs[TaskReg][VideoPage].value=VideoPageValue;

	/* Reset rom page */
	PageRegs[TaskReg][ROMPage].value=ROMPageValue;
	
	/* Reset IO Page */
	PageRegs[TaskReg][IOPage].value=IOPageValue;
	PageRegs[TaskReg][IOPage+1].value=IOPageValue;

	UpdateBanks(0,IOPage+1);

	/* Map video ram to base of area it can use, that way we can take the literal RA */
	/* from the 6845 without having to mask it ! */
	videoram=&mess_ram[TextVidBasePage*RamPageSize];
}

// Return the value of a page register
READ8_HANDLER( dgn_beta_page_r )
{
	return PageRegs[PIATaskReg][offset].value;
}

// Write to a page register, writes to the register, and then checks to see
// if memory banking is active, if it is, it calls UpdateBanks, to actually
// setup the mappings.

WRITE8_HANDLER( dgn_beta_page_w )
{	
	PageRegs[PIATaskReg][offset].value=data;

#ifdef LOG_PAGE_WRITE
	logerror("PageRegWrite : task=$%X  offset=$%X value=$%X\n",PIATaskReg,offset,data);
#endif

	if (EnableMapRegs) 
	{		
		UpdateBanks(offset,offset);
		if (offset==15)
			UpdateBanks(offset+1,offset+1);
	}
}

/*********************** Memory bank write handlers ************************/
/* These actually write the data to the memory, and not to the page regs ! */
static void dgn_beta_bank_memory(int offset, int data, int bank)
{
	PageRegs[TaskReg][bank].memory[offset]=data;
}

void dgnbeta_ram_b0_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,0);
}

void dgnbeta_ram_b1_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,1);
}

void dgnbeta_ram_b2_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,2);
}

void dgnbeta_ram_b3_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,3);
}

void dgnbeta_ram_b4_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,4);
}

void dgnbeta_ram_b5_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,5);
}

void dgnbeta_ram_b6_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,6);
}

void dgnbeta_ram_b7_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,7);
}

void dgnbeta_ram_b8_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,8);
}

void dgnbeta_ram_b9_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,9);
}

void dgnbeta_ram_bA_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,10);
}

void dgnbeta_ram_bB_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,11);
}

void dgnbeta_ram_bC_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,12);
}

void dgnbeta_ram_bD_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,13);
}

void dgnbeta_ram_bE_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,14);
}

void dgnbeta_ram_bF_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,15);
}

void dgnbeta_ram_bG_w(offs_t offset, UINT8 data)
{
	dgn_beta_bank_memory(offset,data,16);
}

/* 
The keyrow being scanned for any key is the lest significant bit 
of the output shift register that is zero, most of the time there should 
only be one row active e.g.

Shifter		Row being scanned
1111111110	0
1111111101	1
1111111011	2

etc.

Returns row number or -1 if none selected.

2006-12-03, P.Harvey-Smith, modified to scan from msb to lsb, and stop at 
first row with zero, as the beta_test fills the shifter with zeros, rather 
than using a walking zero as the OS-9 driver does. This meant that SelectKeyrow
never moved past the first row, by scanning for the last active row
the beta_test rom works, and it does not break the OS-9 driver :)
*/
int SelectedKeyrow(int	Rows)
{
	int	Idx;
	int	Row;	/* Row selected */
	int	Mask;	/* Mask to test row */
	int	Found;	/* Set true when found */
	
	Row=INVALID_KEYROW;	/* Pretend no rows selected */
	Mask=0x200;		/* Start with row 9 */
	Found=0;		/* Start with not found */
	Idx=9;
	
	while ((Mask>0) && !Found)
	{
		if((~Rows & Mask) && !Found)
		{
			Row=Idx;		/* Get row */
			Found=1;		/* Mark as found */
		}
		Idx=Idx-1;			/* Decrement row count */
		Mask=Mask>>1;			/* Select next bit */
	}
		
	return Row;
}

/* GetKeyRow, returns the value of a keyrow, checking for invalid rows */
/* and returning no key pressed if row is invalid */
int GetKeyRow(int RowNo)
{
	if(RowNo==INVALID_KEYROW)
	{
		return NO_KEY_PRESSED;	/* row is invalid, so return no key down */
	}
	else
	{
		return Keyboard[RowNo];	/* Else return keyboard data */
	}
}

/*********************************** PIA Handlers ************************/
/* PIA #0 at $FC20-$FC23 I46
	This handles:-
		The Printer port, side A,
		PB0	R16 (pullup) -> Printer Port (PL1)
		PB1	Printer Port
		PB2 	Keyboard (any key).
		PB3	D4 -> R37 -> TR1 switching circuit -> PL5
		PB4	Keyboard Data out, clocked by CB2.
		        positive edge clocks data out of the input shift register.
		PB5	Keyboard data in, clocked by PB4.
		PB6	R79 -> I99/6/7416 -> PL9/26/READY (from floppy)
		PB7	Printer port
		CB1	I36/39/6845(Horz Sync)
		CB2	Keyboard (out) Low loads input shift reg
*/
static READ8_HANDLER(d_pia0_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia0_pa_w)
{
}

static READ8_HANDLER(d_pia0_pb_r)
{
	int RetVal;
	int Idx;
	int Selected;
	
#ifdef LOG_KEYBOARD
	logerror("PB Read\n");
#endif

	KAny_next=0;
	
	Selected=SelectedKeyrow(RowShifter);
	
	/* Scan the whole keyboard, if output shifter is all low */
	/* This actually scans in the keyboard */
	if(RowShifter==0x00)
	{
		for(Idx=0;Idx<NoKeyrows;Idx++)
		{
			switch (Idx)
			{
				case 0 : Keyboard[Idx]=input_port_0_r(0); break;
				case 1 : Keyboard[Idx]=input_port_1_r(0); break;
				case 2 : Keyboard[Idx]=input_port_2_r(0); break;
				case 3 : Keyboard[Idx]=input_port_3_r(0); break;
				case 4 : Keyboard[Idx]=input_port_4_r(0); break;
				case 5 : Keyboard[Idx]=input_port_5_r(0); break;
				case 6 : Keyboard[Idx]=input_port_6_r(0); break;
				case 7 : Keyboard[Idx]=input_port_7_r(0); break;
				case 8 : Keyboard[Idx]=input_port_8_r(0); break;
				case 9 : Keyboard[Idx]=input_port_9_r(0); break;
			}
			
			if(Keyboard[Idx]!=0x7F)
			{
				KAny_next=1;
			}
		}
	}
	else	/* Just scan current row, from previously read values */
	{
		if(GetKeyRow(Selected)!=NO_KEY_PRESSED)
		{
			KAny_next=1;
		}
	}
	
	RetVal = (KInDat_next<<5) | (KAny_next<<2);
	
#ifdef LOG_KEYBOARD
	logerror("FC22=$%02X KAny=%d\n",RetVal,KAny_next);
#endif
	
	return RetVal;
}

static WRITE8_HANDLER(d_pia0_pb_w)
{
	int	InClkState;
	int	OutClkState;
	
#ifdef LOG_KEYBOARD
	logerror("PB Write\n");
#endif
	
	InClkState	= data & KInClk;
	OutClkState	= data & KOutClk;
	
#ifdef LOG_KEYBOARD
	logerror("InClkState=$%02X OldInClkState=$%02X Keyrow=$%02X ",InClkState,(d_pia0_pb_last & KInClk),Keyrow);
#endif

	/* Input clock bit has changed state */
	if ((InClkState) != (d_pia0_pb_last & KInClk))
	{
		/* Clock in bit */
		if(InClkState)
		{
			KInDat_next=(~Keyrow & 0x40)>>6;
			Keyrow = ((Keyrow<<1) | 0x01) & 0x7F ;
#ifdef LOG_KEYBOARD
			logerror("Keyrow=$%02X KInDat_next=%X\n",Keyrow,KInDat_next);
#endif
		}
	}
		
	d_pia0_pb_last=data;
}

static WRITE8_HANDLER(d_pia0_cb2_w)
{
	int	RowNo;
#ifdef LOG_KEYBOARD
	logerror("\nCB2 Write\n");
#endif
	/* load keyrow on rising edge of CB2 */
	if((data==1) && (d_pia0_cb2_last==0))
	{	
		RowNo=SelectedKeyrow(RowShifter);
		Keyrow=GetKeyRow(RowNo);

		/* Output clock rising edge, clock CB2 value into rowshifterlow to high transition */
		/* In the beta the shift registers are a cmos 4015, and a cmos 4013 in series */
		RowShifter = (RowShifter<<1) | ((d_pia0_pb_last & KOutDat)>>4);
		RowShifter &= 0x3FF;
#ifdef LOG_KEYBOARD
		logerror("Rowshifter=$%02X Keyrow=$%02X\n",RowShifter,Keyrow);
		debug_console_printf("rowshifter clocked, value=%3X, RowNo=%d, Keyrow=%2X\n",RowShifter,RowNo,Keyrow);
#endif
	}
	
	d_pia0_cb2_last=data;
}


static void d_pia0_irq_a(int state)
{
	cpu0_recalc_irq(state);
}

static void d_pia0_irq_b(int state)
{
	cpu0_recalc_firq(state);
}

/* PIA #1 at $FC24-$FC27 I63 
	This handles :-
		Mouse + Disk Select on side A
		Halt on DMA CPU		PA7
		Beeper			PB0
		Halt on main CPU	PB1
		Character set select 	PB6
		Baud rate 		PB1..PB5 ????
*/

static READ8_HANDLER(d_pia1_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia1_pa_w)
{
	int	HALT_DMA;

	/* Only play with halt line if halt bit changed since last write */
	if((data & 0x80)!=d_pia1_pa_last)
	{
		/* Bit 7 of $FF24, seems to control HALT on second CPU (through an inverter) */
		if(data & 0x80)
			HALT_DMA=ASSERT_LINE;
		else
			HALT_DMA=CLEAR_LINE;

#ifdef LOG_HALT
		logerror("DMA_CPU HALT=%d\n",HALT_DMA);
#endif
		cpunum_set_input_line(1, INPUT_LINE_HALT, HALT_DMA);

		/* CPU un-halted let it run ! */
		if (HALT_DMA==CLEAR_LINE)
			cpu_yield();
	
		d_pia1_pa_last=data & 0x80;
	}
	
	/* Drive selects are binary encoded on PA0 & PA1 */
	wd17xx_set_drive(~data & DSMask);
	
	/* Set density of WD2797 */
	if (data & DDenCtrl) 
	{
		wd17xx_set_density(DEN_FM_LO);
#ifdef MAME_DEBUG
		if (LOG_DISK)
			logerror("Set density low %d\n",(data & DDenCtrl));
#endif
	}
	else
	{
		wd17xx_set_density(DEN_MFM_LO);
#ifdef MAME_DEBUG
		if (LOG_DISK)
			logerror("Set density high %d\n",(data & DDenCtrl));
#endif
	}
}

static READ8_HANDLER(d_pia1_pb_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia1_pb_w)
{
	int	HALT_CPU;

	/* Only play with halt line if halt bit changed since last write */
	if((data & 0x02)!=d_pia1_pb_last)
	{
		/* Bit 1 of $FF26, seems to control HALT on primary CPU */
		if(data & 0x02)
			HALT_CPU=CLEAR_LINE;
		else
			HALT_CPU=ASSERT_LINE;
#ifdef LOG_HALT
		logerror("MAIN_CPU HALT=%d\n",HALT_CPU);
#endif
		cpunum_set_input_line(0, INPUT_LINE_HALT, HALT_CPU);

		d_pia1_pb_last=data & 0x02;

		/* CPU un-halted let it run ! */
		if (HALT_CPU==CLEAR_LINE)
			cpu_yield();
	}	
}

static void d_pia1_irq_a(int state)
{
	cpu0_recalc_irq(state);
}

static void d_pia1_irq_b(int state)
{
	cpu0_recalc_irq(state);
}

/* PIA #2 at FCC0-FCC3 I28 
	This handles :-
		DAT task select	PA0..PA3
		
		DMA CPU NMI	PA7
		
		Graphics control PB0..PB7 ???
		VSYNC intutrupt CB2
*/
static READ8_HANDLER(d_pia2_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia2_pa_w)
{
	int OldTask;
	int OldEnableMap;
	int NMI;

#ifdef LOG_TASK
	logerror("FCC0 write : $%02X\n",data);
#endif

	/* Bit 7 of $FFC0, seems to control NMI on second CPU */
	NMI=(data & 0x80);
	
	/* only take action if NMI changed */
	if(NMI!=DMA_NMI_LAST) 
	{
#ifdef LOG_INTS
		logerror("cpu1 NMI : %d\n",NMI);
//		debug_console_printf("cpu1 NMI : %d\n",NMI);
#endif
		if(!NMI)
		{
			cpunum_set_input_line(1,INPUT_LINE_NMI,ASSERT_LINE);
			logerror("cpu_yield()\n");
			cpu_yield();	/* Let DMA CPU run */
		}
		else
		{
			cpunum_set_input_line(1,INPUT_LINE_NMI,CLEAR_LINE);
		}

		DMA_NMI_LAST=NMI;	/* Save it for next time */
	}

	OldEnableMap=EnableMapRegs;
	/* Bit 6 seems to enable memory paging */
	if(data & 0x40)
		EnableMapRegs=0;
	else
		EnableMapRegs=1;
	
	/* Bits 0..3 seem to control which task register is selected */
	OldTask=PIATaskReg;
	PIATaskReg=data & 0x0F;

#ifdef LOG_TASK
	logerror("OldTask=$%02X EnableMapRegs=%d OldEnableMap=%d\n",OldTask,EnableMapRegs,OldEnableMap);
#endif

	// Maping was enabled or disabled, select apropreate task reg
	// and map it in
	if (EnableMapRegs!=OldEnableMap)
	{
		if(EnableMapRegs)
		{
			TaskReg=PIATaskReg;
		}
		else
		{
			TaskReg=NoPagingTask;
		}
		UpdateBanks(0,IOPage+1);
	}
	else
	{
		// Update ram banks only if task reg changed and mapping enabled
		if ((PIATaskReg!=OldTask) && (EnableMapRegs))		
		{
			TaskReg=PIATaskReg;
			UpdateBanks(0,IOPage+1);
		}
	}
#ifdef LOG_TASK	
	logerror("TaskReg=$%02X PIATaskReg=$%02X\n",TaskReg,PIATaskReg);
#endif
}

static READ8_HANDLER(d_pia2_pb_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia2_pb_w)
{
	/* Update top video address lines */
	vid_set_gctrl(data);
}

static void d_pia2_irq_a(int state)
{
	logerror("PIA2 IRQ1 state=%02X\n",state);
	cpu0_recalc_irq(state);
}

static void d_pia2_irq_b(int state)
{
	logerror("PIA2 IRQ2 state=%02X\n",state);
	cpu0_recalc_irq(state);
}

/************************************ Recalculate CPU inturrupts ****************************/
/* CPU 0 */
static void cpu0_recalc_irq(int state)
{
	UINT8 pia0_irq_a = pia_get_irq_a(0);
	UINT8 pia1_irq_a = pia_get_irq_a(1);
	UINT8 pia1_irq_b = pia_get_irq_b(1);
	UINT8 pia2_irq_a = pia_get_irq_a(2);
	UINT8 pia2_irq_b = pia_get_irq_b(2);
	UINT8 IRQ;
	
	if (pia0_irq_a || pia1_irq_a || pia1_irq_b || pia2_irq_a || pia2_irq_b)
		IRQ = ASSERT_LINE;
	else
		IRQ = CLEAR_LINE;

	cpunum_set_input_line(0, M6809_IRQ_LINE, IRQ);
#ifdef LOG_INTS
	logerror("cpu0 IRQ : %d\n",IRQ);
#endif
}

static void cpu0_recalc_firq(int state)
{
	UINT8 pia0_irq_b = pia_get_irq_b(0);
	UINT8 FIRQ;
	
	if (pia0_irq_b)
		FIRQ = ASSERT_LINE;
	else
		FIRQ = CLEAR_LINE;
		
	cpunum_set_input_line(0, M6809_FIRQ_LINE, FIRQ);
	
#ifdef LOG_INTS
	logerror("cpu0 FIRQ : %d\n",FIRQ);
#endif
}

/* CPU 1 */

static void cpu1_recalc_firq(int state)
{
	cpunum_set_input_line(1, M6809_FIRQ_LINE, state);
#ifdef LOG_INTS
	logerror("cpu1 FIRQ : %d\n",state);
#endif
}

/********************************************************************************************/
/* Dragon Beta onboard FDC */
/********************************************************************************************/

static void dgnbeta_fdc_callback(wd17xx_state_t event, void *param)
{
	/* The INTRQ line goes through pia2 ca1, in exactly the same way as DRQ from DragonDos does */
	/* DRQ is routed through various logic to the FIRQ inturrupt line on *BOTH* CPUs */
	
	switch(event) 
	{
		case WD17XX_IRQ_CLR:
			pia_2_ca1_w(0,CLEAR_LINE);
			break;
		case WD17XX_IRQ_SET:
			pia_2_ca1_w(0,ASSERT_LINE);
			break;
		case WD17XX_DRQ_CLR:
			/*wd2797_drq=CLEAR_LINE;*/
			cpu1_recalc_firq(CLEAR_LINE);
			break;
		case WD17XX_DRQ_SET:
			/*wd2797_drq=ASSERT_LINE;*/
			cpu1_recalc_firq(ASSERT_LINE);
			break;		
	}

#ifdef MAME_DEBUG
	if (LOG_DISK)
		logerror("dgnbeta_fdc_callback(%d)\n",event);
#endif
}

 READ8_HANDLER(dgnbeta_wd2797_r)
{
	int result = 0;

	switch(offset & 0x03) 
	{
		case 0:
			result = wd17xx_status_r(0);
#ifdef MAME_DEBUG
			if (LOG_DISK)
				logerror("Disk status=%2.2X\n",result);
#endif
			break;
		case 1:
			result = wd17xx_track_r(0);
			break;
		case 2:
			result = wd17xx_sector_r(0);
			break;
		case 3:
			result = wd17xx_data_r(0);
			break;
		default:
			break;
	}
		
	return result;
}

WRITE8_HANDLER(dgnbeta_wd2797_w)
{
    switch(offset & 0x3) 
	{
		case 0:
			/* disk head is encoded in the command byte */
			/* But only for Type 3/4 commands */
			if(data & 0x80)
				wd17xx_set_side((data & 0x02) ? 1 : 0);
			wd17xx_command_w(0, data);
			break;
		case 1:
			wd17xx_track_w(0, data);
			break;
		case 2:
			wd17xx_sector_w(0, data);
			break;
		case 3:
			wd17xx_data_w(0, data);
			break;
	};
}

/* Scan physical keyboard into Keyboard array */
/* gonna try and sync this more closely with hardware as keyboard being scanned */
/* on *EVERY* vblank ! */
void ScanInKeyboard(void)
{
#if 0
	int	Idx;
	int	Row;

#ifdef LOG_KEYBOARD
	logerror("Scanning Host keyboard\n");
#endif

	for(Idx=0;Idx<NoKeyrows;Idx++)
	{
		switch (Idx)
		{
			case 0 : Row=input_port_0_r(0) /*| 0x33*/; break;
			case 1 : Row=input_port_1_r(0); break;
			case 2 : Row=input_port_2_r(0); break;
			case 3 : Row=input_port_3_r(0); break;
			case 4 : Row=input_port_4_r(0); break;
			case 5 : Row=input_port_5_r(0); break;
			case 6 : Row=input_port_6_r(0); break;
			case 7 : Row=input_port_7_r(0); break;
			case 8 : Row=input_port_8_r(0); break;
			case 9 : Row=input_port_9_r(0); break;
			default : Row=0x7F; break;
		}
		Keyboard[Idx]=Row;
#ifdef LOG_KEYBOARD
		logerror("Keyboard[%d]=$%02X\n",Idx,Row);
		if (Row!=0x7F)
		{
			logerror("Found Pressed Key\n");
		}
#endif
	}
#endif
}

/* VBlank inturrupt */
void dgn_beta_frame_interrupt (int data)
{
	/* Set PIA line, so it recognises inturrupt */
	if (!data) 
	{
		pia_2_cb2_w(0,ASSERT_LINE);
	}
	else
	{
		pia_2_cb2_w(0,CLEAR_LINE);
	}
#ifdef LOG_VIDEO
	logerror("Vblank\n");
#endif
	ScanInKeyboard();
}	

void dgn_beta_line_interrupt (int data)
{
//	/* Set PIA line, so it recognises inturrupt */
//	if (data) 
//	{
//		pia_0_cb1_w(0,ASSERT_LINE);
//	}
//	else
//	{
//		pia_0_cb1_w(0,CLEAR_LINE);
//	}
}


/********************************* Machine/Driver Initialization ****************************************/

static void dgnbeta_reset(running_machine *machine)
{
	system_rom = memory_region(REGION_CPU1);

	/* Make sure CPU 1 is started out halted ! */
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	
	/* Reset to task 0, and map banks disabled, so standard memory map */
	/* with ram at $0000-$BFFF, ROM at $C000-FBFF, IO at $FC00-$FEFF */
	/* and ROM at $FF00-$FFFF */
	TaskReg=0;
	PIATaskReg=0;
	EnableMapRegs=0;
	memset(PageRegs,0,sizeof(PageRegs));	/* Reset page registers to 0 */
	SetDefaultTask();

	pia_reset();

	d_pia1_pa_last=0x00;
	d_pia1_pb_last=0x00;
	RowShifter=0x00;			/* shift register to select row */
	Keyrow=0x00;				/* Keyboard row being shifted out */
	d_pia0_pb_last=0x00;			/* Last byte output to pia0 port b */
	d_pia0_cb2_last=0x00;			/* Last state of CB2 */

	KInDat_next=0x00;			/* Next data bit to input */
	KAny_next=0x00;				/* Next value for KAny */
	
	DMA_NMI_LAST=0x80;			/* start with DMA NMI inactive, as pulled up */
//	DMA_NMI=CLEAR_LINE;			/* start with DMA NMI inactive */
	
	wd17xx_reset();
	wd17xx_set_density(DEN_MFM_LO);
	wd17xx_set_drive(0);

	videoram=mess_ram;			/* Point video ram at the start of physical ram */
}


MACHINE_START( dgnbeta )
{
	pia_config(0, &dgnbeta_pia_intf[0]);
	pia_config(1, &dgnbeta_pia_intf[1]);
	pia_config(2, &dgnbeta_pia_intf[2]); 

	init_video();
	
	wd17xx_init(WD_TYPE_179X,dgnbeta_fdc_callback, NULL);
#ifdef MAME_DEBUG
	cpuintrf_set_dasm_override(0,dgnbeta_dasm_override);
#endif /* MAME_DEBUG */

	add_reset_callback(machine, dgnbeta_reset);
	dgnbeta_reset(machine);
#ifdef MAME_DEBUG
	/* setup debug commands */
	if (Machine->debug_mode)
	{
		debug_console_register_command("beta_dat_log", CMDFLAG_NONE, 0, 0, 0,ToggleDatLog);
		debug_console_register_command("beta_key_dump", CMDFLAG_NONE, 0, 0, 0,DumpKeys);
	}
	LogDatWrites=0;
#endif	/* MAME_DEBUG */
	return 0;
}



/***************************************************************************
  OS9 Syscalls for disassembly
****************************************************************************/

#ifdef MAME_DEBUG

static const char *os9syscalls[] =
{
	"F$Link",          /* Link to Module */
	"F$Load",          /* Load Module from File */
	"F$UnLink",        /* Unlink Module */
	"F$Fork",          /* Start New Process */
	"F$Wait",          /* Wait for Child Process to Die */
	"F$Chain",         /* Chain Process to New Module */
	"F$Exit",          /* Terminate Process */
	"F$Mem",           /* Set Memory Size */
	"F$Send",          /* Send Signal to Process */
	"F$Icpt",          /* Set Signal Intercept */
	"F$Sleep",         /* Suspend Process */
	"F$SSpd",          /* Suspend Process */
	"F$ID",            /* Return Process ID */
	"F$SPrior",        /* Set Process Priority */
	"F$SSWI",          /* Set Software Interrupt */
	"F$PErr",          /* Print Error */
	"F$PrsNam",        /* Parse Pathlist Name */
	"F$CmpNam",        /* Compare Two Names */
	"F$SchBit",        /* Search Bit Map */
	"F$AllBit",        /* Allocate in Bit Map */
	"F$DelBit",        /* Deallocate in Bit Map */
	"F$Time",          /* Get Current Time */
	"F$STime",         /* Set Current Time */
	"F$CRC",           /* Generate CRC */
	"F$GPrDsc",        /* get Process Descriptor copy */
	"F$GBlkMp",        /* get System Block Map copy */
	"F$GModDr",        /* get Module Directory copy */
	"F$CpyMem",        /* Copy External Memory */
	"F$SUser",         /* Set User ID number */
	"F$UnLoad",        /* Unlink Module by name */
	"F$Alarm",         /* Color Computer Alarm Call (system wide) */
	NULL,
	NULL,
	"F$NMLink",        /* Color Computer NonMapping Link */
	"F$NMLoad",        /* Color Computer NonMapping Load */
	NULL,
	NULL,
	"F$TPS",           /* Return System's Ticks Per Second */
	"F$TimAlm",        /* COCO individual process alarm call */
	"F$VIRQ",          /* Install/Delete Virtual IRQ */
	"F$SRqMem",        /* System Memory Request */
	"F$SRtMem",        /* System Memory Return */
	"F$IRQ",           /* Enter IRQ Polling Table */
	"F$IOQu",          /* Enter I/O Queue */
	"F$AProc",         /* Enter Active Process Queue */
	"F$NProc",         /* Start Next Process */
	"F$VModul",        /* Validate Module */
	"F$Find64",        /* Find Process/Path Descriptor */
	"F$All64",         /* Allocate Process/Path Descriptor */
	"F$Ret64",         /* Return Process/Path Descriptor */
	"F$SSvc",          /* Service Request Table Initialization */
	"F$IODel",         /* Delete I/O Module */
	"F$SLink",         /* System Link */
	"F$Boot",          /* Bootstrap System */
	"F$BtMem",         /* Bootstrap Memory Request */
	"F$GProcP",        /* Get Process ptr */
	"F$Move",          /* Move Data (low bound first) */
	"F$AllRAM",        /* Allocate RAM blocks */
	"F$AllImg",        /* Allocate Image RAM blocks */
	"F$DelImg",        /* Deallocate Image RAM blocks */
	"F$SetImg",        /* Set Process DAT Image */
	"F$FreeLB",        /* Get Free Low Block */
	"F$FreeHB",        /* Get Free High Block */
	"F$AllTsk",        /* Allocate Process Task number */
	"F$DelTsk",        /* Deallocate Process Task number */
	"F$SetTsk",        /* Set Process Task DAT registers */
	"F$ResTsk",        /* Reserve Task number */
	"F$RelTsk",        /* Release Task number */
	"F$DATLog",        /* Convert DAT Block/Offset to Logical */
	"F$DATTmp",        /* Make temporary DAT image (Obsolete) */
	"F$LDAXY",         /* Load A [X,[Y]] */
	"F$LDAXYP",        /* Load A [X+,[Y]] */
	"F$LDDDXY",        /* Load D [D+X,[Y]] */
	"F$LDABX",         /* Load A from 0,X in task B */
	"F$STABX",         /* Store A at 0,X in task B */
	"F$AllPrc",        /* Allocate Process Descriptor */
	"F$DelPrc",        /* Deallocate Process Descriptor */
	"F$ELink",         /* Link using Module Directory Entry */
	"F$FModul",        /* Find Module Directory Entry */
	"F$MapBlk",        /* Map Specific Block */
	"F$ClrBlk",        /* Clear Specific Block */
	"F$DelRAM",        /* Deallocate RAM blocks */
	"F$GCMDir",        /* Pack module directory */
	"F$AlHRam",        /* Allocate HIGH RAM Blocks */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F$RegDmp",        /* Ron Lammardo's debugging register dump call */
	"F$NVRAM",         /* Non Volatile RAM (RTC battery backed static) read/write */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"I$Attach",        /* Attach I/O Device */
	"I$Detach",        /* Detach I/O Device */
	"I$Dup",           /* Duplicate Path */
	"I$Create",        /* Create New File */
	"I$Open",          /* Open Existing File */
	"I$MakDir",        /* Make Directory File */
	"I$ChgDir",        /* Change Default Directory */
	"I$Delete",        /* Delete File */
	"I$Seek",          /* Change Current Position */
	"I$Read",          /* Read Data */
	"I$Write",         /* Write Data */
	"I$ReadLn",        /* Read Line of ASCII Data */
	"I$WritLn",        /* Write Line of ASCII Data */
	"I$GetStt",        /* Get Path Status */
	"I$SetStt",        /* Set Path Status */
	"I$Close",         /* Close Path */
	"I$DeletX"         /* Delete from current exec dir */
};


static offs_t dgnbeta_dasm_override(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	unsigned call;
	unsigned result = 0;

	if ((oprom[0] == 0x10) && (oprom[1] == 0x3F))
	{
		call = oprom[2];
		if ((call >= 0) && (call < sizeof(os9syscalls) / sizeof(os9syscalls[0])) && os9syscalls[call])
		{
			sprintf(buffer, "OS9   %s", os9syscalls[call]);
			result = 3;
		}
	}
	return result;
}

#ifdef MAME_DEBUG
static void ToggleDatLog(int ref, int params, const char *param[])
{
	LogDatWrites=!LogDatWrites;

	debug_console_printf("DAT register write info set : %d\n",LogDatWrites);
}

static void DumpKeys(int ref, int params, const char *param[])
{
	int Idx;
	
	for(Idx=0;Idx<NoKeyrows;Idx++)
	{
		debug_console_printf("KeyRow[%d]=%2X\n",Idx,Keyboard[Idx]);
	}
}
#endif

#endif /* MAME_DEBUG */
