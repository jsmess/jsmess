/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

***************************************************************************/

#include <assert.h>
#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/uart8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/pc_turbo.h"

#include "video/generic.h"
#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_aga.h"
#include "video/pc_mda.h"
#include "video/pc_t1t.h"

#include "machine/pit8253.h"

#include "includes/pc_mouse.h"
#include "machine/pckeybrd.h"

#include "includes/pclpt.h"
#include "includes/centroni.h"

#include "machine/pc_hdc.h"
#include "machine/nec765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "includes/ibmpc.h"
#include "machine/pcshare.h"

#include "includes/pc.h"
#include "mscommon.h"

#include "machine/8237dma.h"

DRIVER_INIT( pccga )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	ppi8255_init(&pc_ppi8255_interface);
	pc_rtc_init();
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( bondwell )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	ppi8255_init(&pc_ppi8255_interface);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( pcmda )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	ppi8255_init(&pc_ppi8255_interface);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( europc ) 
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	UINT8 *rom = &memory_region(REGION_CPU1)[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
	  fix century rom bios bug !
	  if year <79 month (and not CENTURY) is loaded with 0x20
	*/
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);

	europc_rtc_init();
//	europc_rtc_set_time();
}

DRIVER_INIT( t1000hx )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( pc200 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc200_videoram_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc200_videoram_w );
	videoram_size=0x10000;
	videoram=memory_region(REGION_CPU1)+0xb0000;
/* This is all done in video_start_pc200()
	// 0x3dd, 0x3d8, 0x3d4, 0x3de are also in mda mode present!?
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc200_cga_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc200_cga_w );

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_w );
*/
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc200_port378_r );

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
}

DRIVER_INIT( pc1512 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0, MRA8_BANK1 );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0, pc1512_videoram_w );

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1512_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1512_w );

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc_parallelport2_r );


	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	mc146818_init(MC146818_IGNORE_CENTURY);
}



static void pc_map_vga_memory(offs_t begin, offs_t end, read8_handler rh, write8_handler wh)
{
	int buswidth;
	buswidth = cputype_databus_width(Machine->drv->cpu[0].cpu_type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, MRA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, MWA8_NOP);

			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, rh);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, wh);
			break;
	}
}



static const struct pc_vga_interface vga_interface =
{
	1,
	pc_map_vga_memory,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};



DRIVER_INIT( pc1640 )
{
	pc_vga_init(&vga_interface, NULL);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0000, 0xaffff, 0, 0, MRA8_BANK1 );
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb7fff, 0, 0, MRA8_BANK2 );
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, MRA8_BANK3 );

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0000, 0xaffff, 0, 0, MWA8_BANK1 );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb7fff, 0, 0, MWA8_BANK2 );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, MWA8_BANK3 );

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, vga_port_03b0_r );
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, paradise_ega_03c0_r );
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1640_port3d0_r );

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, vga_port_03b0_w );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, vga_port_03c0_w );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, vga_port_03d0_w );

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc1640_port278_r );
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x4278, 0x427b, 0, 0, pc1640_port4278_r );

	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);

	mc146818_init(MC146818_IGNORE_CENTURY);
}

DRIVER_INIT( pc_vga )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
	ppi8255_init(&pc_ppi8255_interface);

	pc_vga_init(&vga_interface, NULL);
}

static int pc_irq_callback(int irqline)
{
	return pic8259_acknowledge(0);
}

MACHINE_RESET( pc_mda )
{
	dma8237_reset();
	cpunum_set_irq_callback(0, pc_irq_callback);
}

MACHINE_RESET( pc_cga )
{
	dma8237_reset();
	cpunum_set_irq_callback(0, pc_irq_callback);
}

MACHINE_RESET( pc_t1t )
{
	pc_t1t_reset();
	dma8237_reset();
	cpunum_set_irq_callback(0, pc_irq_callback);
}

MACHINE_RESET( pc_aga )
{
	dma8237_reset();
	cpunum_set_irq_callback(0, pc_irq_callback);
}

MACHINE_RESET( pc_vga )
{
	dma8237_reset();
	cpunum_set_irq_callback(0, pc_irq_callback);
}

/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/
static void pc_generic_frame_interrupt(void (*pc_timer)(void))
{
	if (pc_timer)
		pc_timer();

	pc_keyboard();
}

void pc_mda_frame_interrupt (void)
{
	pc_generic_frame_interrupt(pc_mda_timer);
}

void pc_cga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(NULL);
}

void tandy1000_frame_interrupt (void)
{
	pc_generic_frame_interrupt(pc_t1t_timer);
}

void pc_aga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(pc_aga_timer);
}

void pc_vga_frame_interrupt (void)
{
	pc_generic_frame_interrupt(NULL /* vga_timer */);
}

