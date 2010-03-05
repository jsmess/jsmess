/***************************************************************************

  gba.c

  Driver file to handle emulation of the Nintendo Game Boy Advance.

  By R. Belmont & Harmony

  TODO:
  - Raster timing issues.  Castlevania (HOD and AOS)'s raster effects
    work if the VBlank is fired on scanline 0, else they're offset by
    the height of the vblank region.  Is scanline 0 the start of the
    visible area?

***************************************************************************/

#include "emu.h"
#include "state.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"
#include "devices/cartslot.h"
#include "includes/gba.h"
#include "includes/gb.h"
#include "sound/dac.h"

#define VERBOSE_LEVEL	(2)

INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(devtag_get_device(machine, "maincpu")), buf );
	}
}

static UINT32 timer_clks[4] = { 16777216, 16777216/64, 16777216/256, 16777216/1024 };

static UINT8 *nvptr;
static UINT32 nvsize = 0;
static running_device *nvimage;

static void gba_machine_stop(running_machine *machine)
{
	// only do this if the cart loader detected some form of backup
	if (nvsize > 0)
	{
		image_battery_save(nvimage, nvptr, nvsize);
	}
}

static PALETTE_INIT( gba )
{
	UINT8 r, g, b;
	for( b = 0; b < 32; b++ )
	{
		for( g = 0; g < 32; g++ )
		{
			for( r = 0; r < 32; r++ )
			{
				palette_set_color_rgb( machine, ( b << 10 ) | ( g << 5 ) | r, pal5bit(r), pal5bit(g), pal5bit(b) );
			}
		}
	}
}

static void dma_exec(running_machine *machine, FPTR ch);

static void gba_request_irq(running_machine *machine, UINT32 int_type)
{
	gba_state *state = (gba_state *)machine->driver_data;

	// set flag for later recovery
	state->IF |= int_type;

	// is this specific interrupt enabled?
	int_type &= state->IE;
	if (int_type != 0)
	{
		// master enable?
		if (state->IME & 1)
		{
			cpu_set_input_line(devtag_get_device(machine, "maincpu"), ARM7_IRQ_LINE, ASSERT_LINE);
			cpu_set_input_line(devtag_get_device(machine, "maincpu"), ARM7_IRQ_LINE, CLEAR_LINE);
		}
	}
}

static TIMER_CALLBACK( dma_complete )
{
	int ctrl;
	FPTR ch;
	static UINT32 ch_int[4] = { INT_DMA0, INT_DMA1, INT_DMA2, INT_DMA3 };
	gba_state *state = (gba_state *)machine->driver_data;

	ch = param;

//  printf("dma complete: ch %d\n", ch);

	timer_adjust_oneshot(state->dma_timer[ch], attotime_never, 0);

	ctrl = state->dma_regs[(ch*3)+2] >> 16;

	// IRQ
	if (ctrl & 0x4000)
	{
		gba_request_irq(machine, ch_int[ch]);
	}

	// if we're supposed to repeat, don't clear "active" and then the next vbl/hbl will retrigger us
	// always clear active for immediate DMAs though
	if (!((ctrl>>9) & 1) || ((ctrl & 0x3000) == 0))
	{
//      printf("clear active for ch %d\n", ch);
		state->dma_regs[(ch*3)+2] &= ~0x80000000;	// clear "active" bit
	}
	else
	{
		// if repeat, reload the count
		if ((ctrl>>9) & 1)
		{
			state->dma_cnt[ch] = state->dma_regs[(ch*3)+2]&0xffff;

			// if increment & reload mode, reload the destination
			if (((ctrl>>5)&3) == 3)
			{
				state->dma_dst[ch] = state->dma_regs[(ch*3)+1];
			}
		}
	}
}

static void dma_exec(running_machine *machine, FPTR ch)
{
	int i, cnt;
	int ctrl;
	int srcadd, dstadd;
	UINT32 src, dst;
	const address_space *space = cpu_get_address_space(devtag_get_device(machine, "maincpu"), ADDRESS_SPACE_PROGRAM);
	gba_state *state = (gba_state *)machine->driver_data;

	src = state->dma_src[ch];
	dst = state->dma_dst[ch];
	ctrl = state->dma_regs[(ch*3)+2] >> 16;
	srcadd = state->dma_srcadd[ch];
	dstadd = state->dma_dstadd[ch];

	cnt = state->dma_cnt[ch];
	if (!cnt)
	{
		if (ch == 3)
		{
			cnt = 0x10000;
		}
		else
		{
			cnt = 0x4000;
		}
	}

	// override special parameters
	if ((ctrl & 0x3000) == 0x3000)		// special xfer mode
	{
		switch (ch)
		{
			case 1:			// Ch 1&2 are for audio DMA
			case 2:
				dstadd = 2;	// don't increment destination
				cnt = 4;	// always transfer 4 32-bit words
				ctrl |= 0x400;	// always 32-bit
				break;

			case 3:
				printf("Unsupported DMA 3 special mode\n");
				break;
		}
	}
	else
	{
//      if (dst >= 0x6000000 && dst <= 0x6017fff)
//          printf("DMA exec: ch %d from %08x to %08x, mode %04x, count %04x (PC %x) (%s)\n", (int)ch, src, dst, ctrl, cnt, activecpu_get_pc(), ((ctrl>>10) & 1) ? "32" : "16");
	}

	for (i = 0; i < cnt; i++)
	{
		if ((ctrl>>10) & 1)
		{
			src &= 0xfffffffc;
			dst &= 0xfffffffc;

			// 32-bit
			memory_write_dword(space, dst, memory_read_dword(space, src));
			switch (dstadd)
			{
				case 0:	// increment
					dst += 4;
					break;
				case 1:	// decrement
					dst -= 4;
					break;
				case 2:	// don't move
					break;
				case 3:	// increment and reload
					dst += 4;
					break;
			}
			switch (srcadd)
			{
				case 0:	// increment
					src += 4;
					break;
				case 1:	// decrement
					src -= 4;
					break;
				case 2:	// don't move
					break;
				case 3:	// not used
					printf("DMA: Bad srcadd 3!\n");
					src += 4;
					break;
			}
		}
		else
		{
			src &= 0xfffffffe;
			dst &= 0xfffffffe;

			// 16-bit
			memory_write_word(space, dst, memory_read_word(space, src));
			switch (dstadd)
			{
				case 0:	// increment
					dst += 2;
					break;
				case 1:	// decrement
					dst -= 2;
					break;
				case 2:	// don't move
					break;
				case 3:	// increment and reload
					dst += 2;
					break;
			}
			switch (srcadd)
			{
				case 0:	// increment
					src += 2;
					break;
				case 1:	// decrement
					src -= 2;
					break;
				case 2:	// don't move
					break;
				case 3:	// not used
					printf("DMA: Bad srcadd 3!\n");
					break;
			}
		}
	}

	state->dma_src[ch] = src;
	state->dma_dst[ch] = dst;

//  printf("settng DMA timer %d for %d cycs (tmr %x)\n", ch, cnt, (UINT32)state->dma_timer[ch]);
//  timer_adjust_oneshot(state->dma_timer[ch], ATTOTIME_IN_CYCLES(0, cnt), ch);
	dma_complete(machine, NULL, ch);
}

static void audio_tick(running_machine *machine, int ref)
{
	gba_state *state = (gba_state *)machine->driver_data;

	if (!(state->SOUNDCNT_X & 0x80))
	{
		return;
	}

	if (!ref)
	{
		if (state->fifo_a_ptr != state->fifo_a_in)
		{
			if (state->fifo_a_ptr == 17)
			{
				state->fifo_a_ptr = 0;
			}

			if (state->SOUNDCNT_H & 0x100)
			{
				running_device *dac_device = devtag_get_device(machine, "direct_a_left");

				dac_signed_data_w(dac_device, state->fifo_a[state->fifo_a_ptr]^0x80);
			}
			if (state->SOUNDCNT_H & 0x200)
			{
				running_device *dac_device = devtag_get_device(machine, "direct_a_right");

				dac_signed_data_w(dac_device, state->fifo_a[state->fifo_a_ptr]^0x80);
			}
			state->fifo_a_ptr++;
		}

		// fifo empty?
		if (state->fifo_a_ptr == state->fifo_a_in)
		{
			// is a DMA set up to feed us?
			if ((state->dma_regs[(1*3)+1] == 0x40000a0) && ((state->dma_regs[(1*3)+2] & 0x30000000) == 0x30000000))
			{
				// channel 1 it is
				dma_exec(machine, 1);
			}
			if ((state->dma_regs[(2*3)+1] == 0x40000a0) && ((state->dma_regs[(2*3)+2] & 0x30000000) == 0x30000000))
			{
				// channel 2 it is
				dma_exec(machine, 2);
			}
		}
	}
	else
	{
		if (state->fifo_b_ptr != state->fifo_b_in)
		{
			if (state->fifo_b_ptr == 17)
			{
				state->fifo_b_ptr = 0;
			}

			if (state->SOUNDCNT_H & 0x1000)
			{
				running_device *dac_device = devtag_get_device(machine, "direct_b_left");

				dac_signed_data_w(dac_device, state->fifo_b[state->fifo_b_ptr]^0x80);
			}
			if (state->SOUNDCNT_H & 0x2000)
			{
				running_device *dac_device = devtag_get_device(machine, "direct_b_right");

				dac_signed_data_w(dac_device, state->fifo_b[state->fifo_b_ptr]^0x80);
			}
			state->fifo_b_ptr++;
		}

		if (state->fifo_b_ptr == state->fifo_b_in)
		{
			// is a DMA set up to feed us?
			if ((state->dma_regs[(1*3)+1] == 0x40000a4) && ((state->dma_regs[(1*3)+2] & 0x30000000) == 0x30000000))
			{
				// channel 1 it is
				dma_exec(machine, 1);
			}
			if ((state->dma_regs[(2*3)+1] == 0x40000a4) && ((state->dma_regs[(2*3)+2] & 0x30000000) == 0x30000000))
			{
				// channel 2 it is
				dma_exec(machine, 2);
			}
		}
	}
}

static TIMER_CALLBACK(timer_expire)
{
	static UINT32 tmr_ints[4] = { INT_TM0_OVERFLOW, INT_TM1_OVERFLOW, INT_TM2_OVERFLOW, INT_TM3_OVERFLOW };
	FPTR tmr = (FPTR) param;
	gba_state *state = (gba_state *)machine->driver_data;

//  printf("Timer %d expired, SOUNDCNT_H %04x\n", tmr, state->SOUNDCNT_H);

	// check if timers 0 or 1 are feeding directsound
	if (tmr == 0)
	{
		if ((state->SOUNDCNT_H & 0x400) == 0)
		{
			audio_tick(machine, 0);
		}

		if ((state->SOUNDCNT_H & 0x4000) == 0)
		{
			audio_tick(machine, 1);
		}
	}

	if (tmr == 1)
	{
		if ((state->SOUNDCNT_H & 0x400) == 0x400)
		{
			audio_tick(machine, 0);
		}

		if ((state->SOUNDCNT_H & 0x4000) == 0x4000)
		{
			audio_tick(machine, 1);
		}
	}

	// Handle count-up timing
	switch (tmr)
	{
	case 0:
	        if (state->timer_regs[1] & 0x40000)
	        {
		        state->timer_regs[1] = ( ( state->timer_regs[1] & 0x0000ffff ) + 1 ) & 0x0000ffff;
		        if( ( state->timer_regs[1] & 0x0000ffff ) == 0 )
		        {
		                state->timer_regs[1] |= state->timer_reload[1];
		                if( ( state->timer_regs[1] & 0x400000 ) && ( state->IME != 0 ) )
		                {
			                gba_request_irq( machine, tmr_ints[1] );
		                }
		                if( ( state->timer_regs[2] & 0x40000 ) )
		                {
			                state->timer_regs[2] = ( ( state->timer_regs[2] & 0x0000ffff ) + 1 ) & 0x0000ffff;
			                if( ( state->timer_regs[2] & 0x0000ffff ) == 0 )
			                {
			                        state->timer_regs[2] |= state->timer_reload[2];
			                        if( ( state->timer_regs[2] & 0x400000 ) && ( state->IME != 0 ) )
			                        {
				                        gba_request_irq( machine, tmr_ints[2] );
			                        }
			                        if( ( state->timer_regs[3] & 0x40000 ) )
			                        {
				                        state->timer_regs[3] = ( ( state->timer_regs[3] & 0x0000ffff ) + 1 ) & 0x0000ffff;
				                        if( ( state->timer_regs[3] & 0x0000ffff ) == 0 )
				                        {
				                                state->timer_regs[3] |= state->timer_reload[3];
				                                if( ( state->timer_regs[3] & 0x400000 ) && ( state->IME != 0 ) )
				                                {
					                                gba_request_irq( machine, tmr_ints[3] );
				                                }
				                        }
			                        }
			                }
		                }
		        }
	        }
	        break;
	case 1:
	        if (state->timer_regs[2] & 0x40000)
	        {
		        state->timer_regs[2] = ( ( state->timer_regs[2] & 0x0000ffff ) + 1 ) & 0x0000ffff;
		        if( ( state->timer_regs[2] & 0x0000ffff ) == 0 )
		        {
		                state->timer_regs[2] |= state->timer_reload[2];
		                if( ( state->timer_regs[2] & 0x400000 ) && ( state->IME != 0 ) )
		                {
			                gba_request_irq( machine, tmr_ints[2] );
		                }
		                if( ( state->timer_regs[3] & 0x40000 ) )
		                {
		        	        state->timer_regs[3] = ( ( state->timer_regs[3] & 0x0000ffff ) + 1 ) & 0x0000ffff;
			                if( ( state->timer_regs[3] & 0x0000ffff ) == 0 )
			                {
			                        state->timer_regs[2] |= state->timer_reload[2];
			                        if( ( state->timer_regs[3] & 0x400000 ) && ( state->IME != 0 ) )
			                        {
				                        gba_request_irq( machine, tmr_ints[3] );
			                        }
			                }
		                }
		        }
	        }
	        break;
	case 2:
	        if (state->timer_regs[3] & 0x40000)
	        {
		        state->timer_regs[3] = ( ( state->timer_regs[3] & 0x0000ffff ) + 1 ) & 0x0000ffff;
		        if( ( state->timer_regs[3] & 0x0000ffff ) == 0 )
		        {
		                state->timer_regs[3] |= state->timer_reload[3];
		                if( ( state->timer_regs[3] & 0x400000 ) && ( state->IME != 0 ) )
		                {
			                gba_request_irq( machine, tmr_ints[3] );
		                }
		        }
	        }
	        break;
	}

	// are we supposed to IRQ?
	if ((state->timer_regs[tmr] & 0x400000) && (state->IME != 0))
	{
		gba_request_irq(machine, tmr_ints[tmr]);
	}
}

static TIMER_CALLBACK(handle_irq)
{
	gba_state *state = (gba_state *)machine->driver_data;

	gba_request_irq(machine, state->IF);

	timer_adjust_oneshot(state->irq_timer, attotime_never, 0);
}

static READ32_HANDLER( gba_io_r )
{
	UINT32 retval = 0;
	running_machine *machine = space->machine;
	running_device *gb_device = devtag_get_device(space->machine, "custom");
	gba_state *state = (gba_state *)machine->driver_data;

	switch( offset )
	{
		case 0x0000/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: DISPCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->DISPCNT );
				retval |= state->DISPCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: Green Swap (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->GRNSWAP );
				retval |= state->GRNSWAP << 16;
			}
			break;
		case 0x0004/4:
			retval = (state->DISPSTAT & 0xffff) | (video_screen_get_vpos(machine->primary_screen)<<16);
			break;
		case 0x0008/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG0CNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->BG0CNT );
				retval |= state->BG0CNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG1CNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->BG1CNT );
				retval |= state->BG1CNT << 16;
			}
			break;
		case 0x000c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2CNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->BG2CNT );
				retval |= state->BG2CNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3CNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->BG3CNT );
				retval |= state->BG3CNT << 16;
			}
			break;
		case 0x0010/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG0HOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG0VOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0014/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG1HOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG1VOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0018/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2HOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2VOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x001c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3HOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3VOFS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0020/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2PA (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2PB (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0024/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2PC (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2PD (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0028/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2X_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2X_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x002c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2Y_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG2Y_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0030/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3PA (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3PB (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0034/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3PC (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3PD (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0038/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3X_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3X_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x003c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3Y_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BG3Y_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0040/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: WIN0H (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: WIN1H (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0044/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: WIN0V (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: WIN1V (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0048/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: WININ (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->WININ );
				retval |= state->WININ;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: WINOUT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->WINOUT );
				retval |= state->WINOUT << 16;
			}
			break;
		case 0x004c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: MOSAIC (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0050/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BLDCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->BLDCNT );
				retval |= state->BLDCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: BLDALPHA (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0054/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: BLDY (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0058/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x005c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), 0 );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0060/4:
			retval = gb_sound_r(gb_device, 0) | gb_sound_r(gb_device, 1)<<16 | gb_sound_r(gb_device, 2)<<24;
			break;
		case 0x0064/4:
			retval = gb_sound_r(gb_device, 3) | gb_sound_r(gb_device, 4)<<8;
			break;
		case 0x0068/4:
			retval = gb_sound_r(gb_device, 6) | gb_sound_r(gb_device, 7)<<8;
			break;
		case 0x006c/4:
			retval = gb_sound_r(gb_device, 8) | gb_sound_r(gb_device, 9)<<8;
			break;
		case 0x0070/4:
			retval = gb_sound_r(gb_device, 0xa) | gb_sound_r(gb_device, 0xb)<<16 | gb_sound_r(gb_device, 0xc)<<24;
			break;
		case 0x0074/4:
			retval = gb_sound_r(gb_device, 0xd) | gb_sound_r(gb_device, 0xe)<<8;
			break;
		case 0x0078/4:
			retval = gb_sound_r(gb_device, 0x10) | gb_sound_r(gb_device, 0x11)<<8;
			break;
		case 0x007c/4:
			retval = gb_sound_r(gb_device, 0x12) | gb_sound_r(gb_device, 0x13)<<8;
			break;
		case 0x0080/4:
			retval = gb_sound_r(gb_device, 0x14) | gb_sound_r(gb_device, 0x15)<<8;
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: SOUNDCNT_H (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->SOUNDCNT_H );
				retval |= state->SOUNDCNT_H << 16;
			}
			break;
		case 0x0084/4:
			retval = gb_sound_r(gb_device, 0x16);
			break;
		case 0x0088/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: SOUNDBIAS (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->SOUNDBIAS );
				retval |= state->SOUNDBIAS;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0090/4:
			retval = gb_wave_r(gb_device, 0) | gb_wave_r(gb_device, 1)<<8 | gb_wave_r(gb_device, 2)<<16 | gb_wave_r(gb_device, 3)<<24;
			break;
		case 0x0094/4:
			retval = gb_wave_r(gb_device, 4) | gb_wave_r(gb_device, 5)<<8 | gb_wave_r(gb_device, 6)<<16 | gb_wave_r(gb_device, 7)<<24;
			break;
		case 0x0098/4:
			retval = gb_wave_r(gb_device, 8) | gb_wave_r(gb_device, 9)<<8 | gb_wave_r(gb_device, 10)<<16 | gb_wave_r(gb_device, 11)<<24;
			break;
		case 0x009c/4:
			retval = gb_wave_r(gb_device, 12) | gb_wave_r(gb_device, 13)<<8 | gb_wave_r(gb_device, 14)<<16 | gb_wave_r(gb_device, 15)<<24;
			break;
		case 0x00a0/4:
		case 0x00a4/4:
			return 0;	// (does this actually do anything on real h/w?)
			break;
		case 0x00b0/4:
		case 0x00b4/4:
		case 0x00b8/4:
		case 0x00bc/4:
		case 0x00c0/4:
		case 0x00c4/4:
		case 0x00c8/4:
		case 0x00cc/4:
		case 0x00d0/4:
		case 0x00d4/4:
		case 0x00d8/4:
		case 0x00dc/4:
			{
				// no idea why here, but it matches VBA better
				if (((offset-0xb0/4) % 3) == 2)
				{
					return state->dma_regs[offset-(0xb0/4)] & 0xff000000;
				}

				return state->dma_regs[offset-(0xb0/4)];
			}
			break;
		case 0x0100/4:
		case 0x0104/4:
		case 0x0108/4:
		case 0x010c/4:
			{
				UINT32 elapsed;
				double time, ticks;
				int timer = offset-(0x100/4);

//              printf("Read timer reg %x (PC=%x)\n", timer, cpu_get_pc(space->cpu));

				// update times for
				if (state->timer_regs[timer] & 0x800000)
				{
					time = attotime_to_double(timer_timeelapsed(state->tmr_timer[timer]));

					ticks = (double)(0x10000 - (state->timer_regs[timer] & 0xffff));

//                  printf("time %f ticks %f 1/hz %f\n", time, ticks, 1.0 / state->timer_hz[timer]);

					time *= ticks;
					time /= (1.0 / state->timer_hz[timer]);

					elapsed = (UINT32)time;

//                  printf("elapsed = %x\n", elapsed);
				}
				else
				{
//                  printf("Reading inactive timer!\n");
					elapsed = 0;
				}

				return (state->timer_regs[timer] & 0xffff0000) | (elapsed & 0xffff);
			}
			break;
		case 0x0120/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIOMULTI0 (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->SIOMULTI0 );
				retval |= state->SIOMULTI0;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIOMULTI1 (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->SIOMULTI1 );
				retval |= state->SIOMULTI1 << 16;
			}
			break;
		case 0x0124/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIOMULTI2 (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->SIOMULTI2 );
				retval |= state->SIOMULTI2;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIOMULTI3 (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->SIOMULTI3 );
				retval |= state->SIOMULTI3 << 16;
			}
			break;
		case 0x0128/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIOCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->SIOCNT );
				retval |= state->SIOCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: SIODATA8 (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->SIODATA8 );
				retval |= state->SIODATA8 << 16;
			}
			break;
		case 0x0130/4:
			if( (mem_mask) & 0x0000ffff )	// KEYINPUT
			{
				return input_port_read(machine, "IN0");
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: KEYCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->KEYCNT );
				retval |= state->KEYCNT << 16;
			}
			break;
		case 0x0134/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: RCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->RCNT );
				retval |= state->RCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: IR (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0140/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOYCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->JOYCNT );
				retval |= state->JOYCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0150/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOY_RECV_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->JOY_RECV & 0x0000ffff );
				retval |= state->JOY_RECV & 0x0000ffff;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOY_RECV_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, ( state->JOY_RECV & 0xffff0000 ) >> 16 );
				retval |= state->JOY_RECV & 0xffff0000;
			}
			break;
		case 0x0154/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOY_TRANS_LSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->JOY_TRANS & 0x0000ffff );
				retval |= state->JOY_TRANS & 0x0000ffff;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOY_TRANS_MSW (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, ( state->JOY_TRANS & 0xffff0000 ) >> 16 );
				retval |= state->JOY_TRANS & 0xffff0000;
			}
			break;
		case 0x0158/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: JOYSTAT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->JOYSTAT );
				retval |= state->JOYSTAT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0200/4:
			if( (mem_mask) & 0x0000ffff )
			{
//              printf("Read: IE (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->IE );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: IF (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, state->IF );
			}

			retval = state->IE | (state->IF<<16);
			break;
		case 0x0204/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: WAITCNT (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->WAITCNT );
				retval |= state->WAITCNT;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0208/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Read: IME (%08x) = %04x\n", 0x04000000 + ( offset << 2 ), state->IME );
				retval |= state->IME;
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Read: UNKNOWN (%08x) = %04x\n", 0x04000000 + ( offset << 2 ) + 2, 0 );
			}
			break;
		case 0x0300/4:
			retval = state->HALTCNT << 8;
			break;
		default:
//          verboselog(machine, 0, "Unknown GBA I/O register Read: %08x (%08x)\n", 0x04000000 + ( offset << 2 ), ~mem_mask );
			break;
	}
	return retval;
}

static WRITE32_HANDLER( gba_io_w )
{
	running_machine *machine = space->machine;
	running_device *gb_device = devtag_get_device(space->machine, "custom");
	gba_state *state = (gba_state *)machine->driver_data;

	switch( offset )
	{
		case 0x0000/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: DISPCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->DISPCNT = ( state->DISPCNT & ~mem_mask ) | ( data & mem_mask );
				if(state->DISPCNT & (DISPCNT_WIN0_EN | DISPCNT_WIN1_EN))
				{
					state->windowOn = 1;
				}
				else
				{
					state->windowOn = 0;
				}
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: Green Swap (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
				state->GRNSWAP = ( state->GRNSWAP & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0004/4:
			COMBINE_DATA(&state->DISPSTAT);
			break;
		case 0x0008/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG0CNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG0CNT = ( state->BG0CNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG1CNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG1CNT = ( state->BG1CNT & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x000c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2CNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG2CNT = ( state->BG2CNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3CNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG3CNT = ( state->BG3CNT & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0010/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG0HOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG0HOFS = ( state->BG0HOFS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG0VOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG0VOFS = ( state->BG0VOFS & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0014/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG1HOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG1HOFS = ( state->BG1HOFS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG1VOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG1VOFS = ( state->BG1VOFS & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0018/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2HOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG2HOFS = ( state->BG2HOFS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2VOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG2VOFS = ( state->BG2VOFS & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x001c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3HOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG3HOFS = ( state->BG3HOFS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3VOFS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG3VOFS = ( state->BG3VOFS & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0020/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2PA (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG2PA = ( state->BG2PA & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2PB (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG2PB = ( state->BG2PB & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0024/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2PC (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG2PC = ( state->BG2PC & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2PD (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG2PD = ( state->BG2PD & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0028/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2X_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2X_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			COMBINE_DATA(&state->BG2X);
			state->gfxBG2Changed |= 1;
			break;
		case 0x002c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2Y_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG2Y_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			COMBINE_DATA(&state->BG2Y);
			state->gfxBG2Changed |= 2;
			break;
		case 0x0030/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3PA (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG3PA = ( state->BG3PA & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3PB (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG3PB = ( state->BG3PB & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0034/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3PC (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BG3PC = ( state->BG3PC & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3PD (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BG3PD = ( state->BG3PD & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0038/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3X_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3X_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			COMBINE_DATA(&state->BG3X);
			state->gfxBG3Changed |= 1;
			break;
		case 0x003c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3Y_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BG3Y_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			COMBINE_DATA(&state->BG3Y);
			state->gfxBG3Changed |= 2;
			break;
		case 0x0040/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: WIN0H (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->WIN0H = ( state->WIN0H & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: WIN1H (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->WIN1H = ( state->WIN1H & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0044/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: WIN0V (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->WIN0V = ( state->WIN0V & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: WIN1V (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->WIN1V = ( state->WIN1V & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0048/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: WININ (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->WININ = ( state->WININ & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: WINOUT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->WINOUT = ( state->WINOUT & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x004c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: MOSAIC (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->MOSAIC = ( state->MOSAIC & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		case 0x0050/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BLDCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BLDCNT = ( state->BLDCNT & ~mem_mask ) | ( data & mem_mask );
				if(state->BLDCNT & BLDCNT_SFX)
				{
					state->fxOn = 1;
				}
				else
				{
					state->fxOn = 0;
				}
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: BLDALPHA (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
                state->BLDALPHA = ( state->BLDALPHA & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0054/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: BLDY (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->BLDY = ( state->BLDY & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		case 0x0058/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		case 0x005c/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		case 0x0060/4:
			if( (mem_mask) & 0x000000ff )	// SOUNDCNTL
			{
				gb_sound_w(gb_device, 0, data);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_sound_w(gb_device, 1, data>>16);	// SOUND1CNT_H
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_sound_w(gb_device, 2, data>>24);
			}
			break;
		case 0x0064/4:
			if( (mem_mask) & 0x000000ff )	// SOUNDCNTL
			{
				gb_sound_w(gb_device, 3, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 4, data>>8);	// SOUND1CNT_H
			}
			break;
		case 0x0068/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 6, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 7, data>>8);
			}
			break;
		case 0x006c/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 8, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 9, data>>8);
			}
			break;
		case 0x0070/4:	//SND3CNTL and H
			if( (mem_mask) & 0x000000ff )	// SOUNDCNTL
			{
				gb_sound_w(gb_device, 0xa, data);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_sound_w(gb_device, 0xb, data>>16);	// SOUND1CNT_H
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_sound_w(gb_device, 0xc, data>>24);
			}
			break;
		case 0x0074/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 0xd, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 0xe, data>>8);
			}
			break;
		case 0x0078/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 0x10, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 0x11, data>>8);
			}
			break;
		case 0x007c/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 0x12, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 0x13, data>>8);
			}
			break;
		case 0x0080/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_sound_w(gb_device, 0x14, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_sound_w(gb_device, 0x15, data>>8);
			}

			if ((mem_mask) & 0xffff0000)
			{
				data >>= 16;
				state->SOUNDCNT_H = data;

				// DAC A reset?
				if (data & 0x0800)
				{
					running_device *gb_a_l = devtag_get_device(machine, "direct_a_left");
					running_device *gb_a_r = devtag_get_device(machine, "direct_a_right");

					state->fifo_a_ptr = 17;
					state->fifo_a_in = 17;
					dac_signed_data_w(gb_a_l, 0x80);
					dac_signed_data_w(gb_a_r, 0x80);
				}

				// DAC B reset?
				if (data & 0x8000)
				{
					running_device *gb_b_l = devtag_get_device(machine, "direct_b_left");
					running_device *gb_b_r = devtag_get_device(machine, "direct_b_right");

					state->fifo_b_ptr = 17;
					state->fifo_b_in = 17;
					dac_signed_data_w(gb_b_l, 0x80);
					dac_signed_data_w(gb_b_r, 0x80);
				}
			}
			break;
		case 0x0084/4:
			if( (mem_mask) & 0x000000ff )
			{
				running_device *gb_a_l = devtag_get_device(machine, "direct_a_left");
				running_device *gb_a_r = devtag_get_device(machine, "direct_a_right");
				running_device *gb_b_l = devtag_get_device(machine, "direct_b_left");
				running_device *gb_b_r = devtag_get_device(machine, "direct_b_right");

				gb_sound_w(gb_device, 0x16, data);
				if ((data & 0x80) && !(state->SOUNDCNT_X & 0x80))
				{
					state->fifo_a_ptr = state->fifo_a_in = 17;
					state->fifo_b_ptr = state->fifo_b_in = 17;
					dac_signed_data_w(gb_a_l, 0x80);
					dac_signed_data_w(gb_a_r, 0x80);
					dac_signed_data_w(gb_b_l, 0x80);
					dac_signed_data_w(gb_b_r, 0x80);
				}
				state->SOUNDCNT_X = data;
			}
			break;
		case 0x0088/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: SOUNDBIAS (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x0000ffff, ~mem_mask );
				state->SOUNDBIAS = ( state->SOUNDBIAS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		case 0x0090/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_wave_w(gb_device, 0, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_wave_w(gb_device, 1, data>>8);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_wave_w(gb_device, 2, data>>16);
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_wave_w(gb_device, 3, data>>24);
			}
			break;
		case 0x0094/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_wave_w(gb_device, 4, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_wave_w(gb_device, 5, data>>8);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_wave_w(gb_device, 6, data>>16);
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_wave_w(gb_device, 7, data>>24);
			}
			break;
		case 0x0098/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_wave_w(gb_device, 8, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_wave_w(gb_device, 9, data>>8);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_wave_w(gb_device, 0xa, data>>16);
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_wave_w(gb_device, 0xb, data>>24);
			}
			break;
		case 0x009c/4:
			if( (mem_mask) & 0x000000ff )
			{
				gb_wave_w(gb_device, 0xc, data);
			}
			if( (mem_mask) & 0x0000ff00 )
			{
				gb_wave_w(gb_device, 0xd, data>>8);
			}
			if( (mem_mask) & 0x00ff0000 )
			{
				gb_wave_w(gb_device, 0xe, data>>16);
			}
			if( (mem_mask) & 0xff000000 )
			{
				gb_wave_w(gb_device, 0xf, data>>24);
			}
			break;
		case 0x00a0/4:
			state->fifo_a_in %= 17;
			state->fifo_a[state->fifo_a_in++] = (data)&0xff;
			state->fifo_a_in %= 17;
			state->fifo_a[state->fifo_a_in++] = (data>>8)&0xff;
			state->fifo_a_in %= 17;
			state->fifo_a[state->fifo_a_in++] = (data>>16)&0xff;
			state->fifo_a_in %= 17;
			state->fifo_a[state->fifo_a_in++] = (data>>24)&0xff;
			break;
		case 0x00a4/4:
			state->fifo_b_in %= 17;
			state->fifo_b[state->fifo_b_in++] = (data)&0xff;
			state->fifo_b_in %= 17;
			state->fifo_b[state->fifo_b_in++] = (data>>8)&0xff;
			state->fifo_b_in %= 17;
			state->fifo_b[state->fifo_b_in++] = (data>>16)&0xff;
			state->fifo_b_in %= 17;
			state->fifo_b[state->fifo_b_in++] = (data>>24)&0xff;
			break;
		case 0x00b0/4:
		case 0x00b4/4:
		case 0x00b8/4:

		case 0x00bc/4:
		case 0x00c0/4:
		case 0x00c4/4:

		case 0x00c8/4:
		case 0x00cc/4:
		case 0x00d0/4:

		case 0x00d4/4:
		case 0x00d8/4:
		case 0x00dc/4:
			{
				int ch;

				offset -= (0xb0/4);

				ch = offset / 3;

//              printf("%08x: DMA(%d): %x to reg %d (mask %08x)\n", activecpu_get_pc(), ch, data, offset%3, ~mem_mask);

				if (((offset % 3) == 2) && ((~mem_mask & 0xffff0000) == 0))
				{
					int ctrl = data>>16;

					// retrigger/restart on a rising edge.
					// also reload internal regs
					// (note: Metroid Fusion fails if we enforce the "rising edge" requirement...)
					if (ctrl & 0x8000) //&& !(state->dma_regs[offset] & 0x80000000))
					{
						state->dma_src[ch] = state->dma_regs[(ch*3)+0];
						state->dma_dst[ch] = state->dma_regs[(ch*3)+1];
						state->dma_srcadd[ch] = (ctrl>>7)&3;
						state->dma_dstadd[ch] = (ctrl>>5)&3;

                        COMBINE_DATA(&state->dma_regs[offset]);
                        state->dma_cnt[ch] = state->dma_regs[(ch*3)+2]&0xffff;

                        // immediate start
						if ((ctrl & 0x3000) == 0)
						{
							dma_exec(machine, ch);
							return;
						}
					}
				}

				COMBINE_DATA(&state->dma_regs[offset]);
			}
			break;
		case 0x0100/4:
		case 0x0104/4:
		case 0x0108/4:
		case 0x010c/4:
			{
				double rate, clocksel;

				offset -= (0x100/4);

				COMBINE_DATA(&state->timer_regs[offset]);

//              printf("%x to timer %d (mask %x PC %x)\n", data, offset, ~mem_mask, cpu_get_pc(space->cpu));

				if (ACCESSING_BITS_0_15)
				{
				        state->timer_reload[offset] = ( state->timer_reload[offset] & ~mem_mask ) | ( ( data & 0x0000ffff ) & mem_mask );
				}

				// enabling this timer?
				if ((ACCESSING_BITS_16_31) && (data & 0x800000))
				{
					double final;

					rate = 0x10000 - (state->timer_regs[offset] & 0xffff);

					clocksel = timer_clks[(state->timer_regs[offset] >> 16) & 3];

					final = clocksel / rate;

					state->timer_hz[offset] = final;

//                  printf("Enabling timer %d @ %f Hz\n", offset, final);

					// enable the timer
					if( !(data & 0x40000) ) // if we're not in Count-Up mode
					{
						timer_adjust_periodic(state->tmr_timer[offset], ATTOTIME_IN_HZ(final), offset, ATTOTIME_IN_HZ(final));
					}
				}
			}
			break;
		case 0x0120/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIOMULTI0 (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->SIOMULTI0 = ( state->SIOMULTI0 & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIOMULTI1 (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->SIOMULTI1 = ( state->SIOMULTI1 & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0124/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIOMULTI2 (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->SIOMULTI2 = ( state->SIOMULTI2 & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIOMULTI3 (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->SIOMULTI3 = ( state->SIOMULTI3 & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0128/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIOCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->SIOCNT = ( state->SIOCNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: SIODATA8 (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->SIODATA8 = ( state->SIODATA8 & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0130/4:
			if( (mem_mask) & 0xffff0000 )
			{
//              printf("KEYCNT = %04x\n", data>>16);
				verboselog(machine, 2, "GBA IO Register Write: KEYCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->KEYCNT = ( state->KEYCNT & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0134/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: RCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->RCNT = ( state->RCNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: IR (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->IR = ( state->IR & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0140/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOYCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->JOYCNT = ( state->JOYCNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
			}
			break;
		case 0x0150/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOY_RECV_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->JOY_RECV = ( state->JOY_RECV & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOY_RECV_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->JOY_RECV = ( state->JOY_RECV & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0154/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOY_TRANS_LSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->JOY_TRANS = ( state->JOY_TRANS & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOY_TRANS_MSW (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
                state->JOY_TRANS = ( state->JOY_TRANS & ( ~mem_mask >> 16 ) ) | ( ( data & mem_mask ) >> 16 );
			}
			break;
		case 0x0158/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: JOYSTAT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->JOYSTAT = ( state->JOYSTAT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
			}
			break;
		case 0x0200/4:
			if( (mem_mask) & 0x0000ffff )
			{
//              printf("IE (%08x) = %04x raw %x (%08x) (scan %d PC %x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, data, ~mem_mask, video_screen_get_vpos(machine->primary_screen), cpu_get_pc(space->cpu));
				state->IE = ( state->IE & ~mem_mask ) | ( data & mem_mask );
#if 0
				if (state->IE & state->IF)
				{
					gba_request_irq(machine, state->IF);
				}
#endif
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: IF (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ) + 2, ( data & mem_mask ) >> 16, ~mem_mask );
				state->IF &= ~( ( data & mem_mask ) >> 16 );

				// if we still have interrupts, yank the IRQ line again
				if (state->IF)
				{
					timer_adjust_oneshot(state->irq_timer, cpu_clocks_to_attotime(devtag_get_device(machine, "maincpu"), 120), 0);
				}
			}
			break;
		case 0x0204/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 2, "GBA IO Register Write: WAITCNT (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->WAITCNT = ( state->WAITCNT & ~mem_mask ) | ( data & mem_mask );
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
			}
			break;
		case 0x0208/4:
			if( (mem_mask) & 0x0000ffff )
			{
				verboselog(machine, 3, "GBA IO Register Write: IME (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), data & mem_mask, ~mem_mask );
				state->IME = ( state->IME & ~mem_mask ) | ( data & mem_mask );
				if (state->IF)
				{
					timer_adjust_oneshot(state->irq_timer, attotime_zero, 0);
				}
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 3, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & mem_mask ) >> 16, ~mem_mask );
			}
			break;
		case 0x0300/4:
			if( (mem_mask) & 0x0000ffff )
			{
				if( (mem_mask) & 0x000000ff )
				{
					verboselog(machine, 2, "GBA IO Register Write: POSTFLG (%08x) = %02x (%08x)\n", 0x04000000 + ( offset << 2 ), data & 0x000000ff, ~mem_mask );
					state->POSTFLG = data & 0x000000ff;
				}
				else
				{
					state->HALTCNT = data & 0x000000ff;

					// either way, wait for an IRQ
					cpu_spinuntil_int(devtag_get_device(machine, "maincpu"));
				}
			}
			if( (mem_mask) & 0xffff0000 )
			{
				verboselog(machine, 2, "GBA IO Register Write: UNKNOWN (%08x) = %04x (%08x)\n", 0x04000000 + ( offset << 2 ), ( data & 0xffff0000 ) >> 16, ~mem_mask );
			}
			break;
		default:
//          verboselog(machine, 0, "Unknown GBA I/O register write: %08x = %08x (%08x)\n", 0x04000000 + ( offset << 2 ), data, ~mem_mask );
			break;
	}
}

INLINE UINT32 COMBINE_DATA32_16(UINT32 prev, UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&prev);
	switch(mem_mask)
	{
		case 0x000000ff:
			prev &= 0xffff00ff;
			prev |= data << 8;
			break;
		case 0x0000ff00:
			prev &= 0xffffff00;
			prev |= data >> 8;
			break;
		case 0x00ff0000:
			prev &= 0x00ffffff;
			prev |= data << 8;
			break;
		case 0xff000000:
			prev &= 0xffffffff;
			prev |= data << 8;
			break;
		default:
			break;
	}
	return prev;
}

static WRITE32_HANDLER(gba_pram_w)
{
	running_machine *machine = space->machine;
	gba_state *state = (gba_state *)machine->driver_data;

	state->gba_pram[offset] = COMBINE_DATA32_16(state->gba_pram[offset], data, mem_mask);
}

static WRITE32_HANDLER(gba_vram_w)
{
	running_machine *machine = space->machine;
	gba_state *state = (gba_state *)machine->driver_data;

	state->gba_vram[offset] = COMBINE_DATA32_16(state->gba_vram[offset], data, mem_mask);
}

static WRITE32_HANDLER(gba_oam_w)
{
	running_machine *machine = space->machine;
	gba_state *state = (gba_state *)machine->driver_data;

	state->gba_oam[offset] = COMBINE_DATA32_16(state->gba_oam[offset], data, mem_mask);
}

static ADDRESS_MAP_START( gbadvance_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00003fff) AM_ROMBANK("bank1")
	AM_RANGE(0x02000000, 0x0203ffff) AM_RAM AM_MIRROR(0xfc0000)
	AM_RANGE(0x03000000, 0x03007fff) AM_RAM AM_MIRROR(0xff8000)
	AM_RANGE(0x04000000, 0x040003ff) AM_READWRITE( gba_io_r, gba_io_w )
	AM_RANGE(0x05000000, 0x050003ff) AM_RAM_WRITE(gba_pram_w) AM_BASE_MEMBER(gba_state, gba_pram)	// Palette RAM
	AM_RANGE(0x06000000, 0x06017fff) AM_RAM_WRITE(gba_vram_w) AM_BASE_MEMBER(gba_state, gba_vram)	// VRAM
	AM_RANGE(0x07000000, 0x070003ff) AM_RAM_WRITE(gba_oam_w) AM_BASE_MEMBER(gba_state, gba_oam)	// OAM
	AM_RANGE(0x08000000, 0x09ffffff) AM_ROM AM_REGION("cartridge", 0)	// cartridge ROM (mirror 0)
	AM_RANGE(0x0a000000, 0x0bffffff) AM_ROM AM_REGION("cartridge", 0)	// cartridge ROM (mirror 1)
	AM_RANGE(0x0c000000, 0x0cffffff) AM_ROM AM_REGION("cartridge", 0)	// final mirror
ADDRESS_MAP_END

static INPUT_PORTS_START( gbadv )
	PORT_START("IN0")
	PORT_BIT( 0xfc00, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_UNUSED
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("P1 R") PORT_PLAYER(1)	// R
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 L") PORT_PLAYER(1)	// L
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START ) PORT_PLAYER(1)	// START
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SELECT ) PORT_PLAYER(1)	// SELECT
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("B") PORT_PLAYER(1)	// B
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("A") PORT_PLAYER(1)	// A
INPUT_PORTS_END

static TIMER_CALLBACK( perform_hbl )
{
	int ch, ctrl;
	gba_state *state = (gba_state *)machine->driver_data;
	int scanline = video_screen_get_vpos(machine->primary_screen);

	// draw only visible scanlines
	if (scanline < 160)
	{
		gba_draw_scanline(machine, scanline);
	}
	state->DISPSTAT |= DISPSTAT_HBL;
	if ((state->DISPSTAT & DISPSTAT_HBL_IRQ_EN ) != 0)
	{
		gba_request_irq(machine, INT_HBL);
	}

	for (ch = 0; ch < 4; ch++)
	{
		ctrl = state->dma_regs[(ch*3)+2]>>16;

		// HBL-triggered DMA?
		if ((ctrl & 0x8000) && ((ctrl & 0x3000) == 0x2000))
		{
			dma_exec(machine, ch);
		}
	}

	timer_adjust_oneshot(state->hbl_timer, attotime_never, 0);
}

static TIMER_CALLBACK( perform_scan )
{
	int scanline;
	gba_state *state = (gba_state *)machine->driver_data;

	// clear hblank and raster IRQ flags
	state->DISPSTAT &= ~(DISPSTAT_HBL|DISPSTAT_VCNT);

	scanline = video_screen_get_vpos(machine->primary_screen);

	// VBL is set for scanlines 160 through 226 (but not 227, which is the last line)
	if (scanline >= 160 && scanline < 227)
	{
		state->DISPSTAT |= DISPSTAT_VBL;
	}
	else
	{
		state->DISPSTAT &= ~DISPSTAT_VBL;
	}

	// handle VCNT match interrupt/flag
	if (scanline == ((state->DISPSTAT >> 8) & 0xff))
	{
		state->DISPSTAT |= DISPSTAT_VCNT;
		if (state->DISPSTAT & DISPSTAT_VCNT_IRQ_EN)
		{
			gba_request_irq(machine, INT_VCNT);
		}
	}

	// entering VBL, handle interrupts and DMA triggers
	if (scanline == 160)
	{
		int ch, ctrl;

		if (state->DISPSTAT & DISPSTAT_VBL_IRQ_EN)
		{
			gba_request_irq(machine, INT_VBL);
		}

		for (ch = 0; ch < 4; ch++)
		{
			ctrl = state->dma_regs[(ch*3)+2]>>16;

			// VBL-triggered DMA?
			if ((ctrl & 0x8000) && ((ctrl & 0x3000) == 0x1000))
			{
				dma_exec(machine, ch);
			}
		}
	}

	timer_adjust_oneshot(state->hbl_timer, video_screen_get_time_until_pos(machine->primary_screen, scanline, 240), 0);
	timer_adjust_oneshot(state->scan_timer, video_screen_get_time_until_pos(machine->primary_screen, ( scanline + 1 ) % 228, 0), 0);
}

static MACHINE_RESET( gba )
{
	running_device *gb_a_l = devtag_get_device(machine, "direct_a_left");
	running_device *gb_a_r = devtag_get_device(machine, "direct_a_right");
	running_device *gb_b_l = devtag_get_device(machine, "direct_b_left");
	running_device *gb_b_r = devtag_get_device(machine, "direct_b_right");
	gba_state *state = (gba_state *)machine->driver_data;

	memset(state, 0, sizeof(state));
	state->SOUNDBIAS = 0x0200;
	state->flash64k_state = FLASH_IDLEBYTE0;
	state->eeprom_state = EEP_IDLE;
	state->SIOMULTI0 = 0xffff;
	state->SIOMULTI1 = 0xffff;
	state->SIOMULTI2 = 0xffff;
	state->SIOMULTI3 = 0xffff;
	state->KEYCNT = 0x03ff;
	state->RCNT = 0x8000;
	state->JOYSTAT = 0x0002;
	state->gfxBG2Changed = 0;
	state->gfxBG3Changed = 0;
	state->gfxBG2X = 0;
	state->gfxBG2Y = 0;
	state->gfxBG3X = 0;
	state->gfxBG3Y = 0;

	state->windowOn = 0;
	state->fxOn = 0;

	timer_adjust_oneshot(state->scan_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0), 0);
	timer_adjust_oneshot(state->hbl_timer, attotime_never, 0);
	timer_adjust_oneshot(state->dma_timer[0], attotime_never, 0);
	timer_adjust_oneshot(state->dma_timer[1], attotime_never, 1);
	timer_adjust_oneshot(state->dma_timer[2], attotime_never, 2);
	timer_adjust_oneshot(state->dma_timer[3], attotime_never, 3);

	state->fifo_a_ptr = state->fifo_b_ptr = 17;	// indicate empty
	state->fifo_a_in = state->fifo_b_in = 17;

	// and clear the DACs
        dac_signed_data_w(gb_a_l, 0x80);
        dac_signed_data_w(gb_a_r, 0x80);
        dac_signed_data_w(gb_b_l, 0x80);
        dac_signed_data_w(gb_b_r, 0x80);
}

static MACHINE_START( gba )
{
	gba_state *state = (gba_state *)machine->driver_data;

	/* add a hook for battery save */
	add_exit_callback(machine, gba_machine_stop);

	/* create a timer to fire scanline functions */
	state->scan_timer = timer_alloc(machine, perform_scan, 0);
	state->hbl_timer = timer_alloc(machine, perform_hbl, 0);
	timer_adjust_oneshot(state->scan_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0), 0);

	/* and one for each DMA channel */
	state->dma_timer[0] = timer_alloc(machine, dma_complete, 0);
	state->dma_timer[1] = timer_alloc(machine, dma_complete, 0);
	state->dma_timer[2] = timer_alloc(machine, dma_complete, 0);
	state->dma_timer[3] = timer_alloc(machine, dma_complete, 0);
	timer_adjust_oneshot(state->dma_timer[0], attotime_never, 0);
	timer_adjust_oneshot(state->dma_timer[1], attotime_never, 1);
	timer_adjust_oneshot(state->dma_timer[2], attotime_never, 2);
	timer_adjust_oneshot(state->dma_timer[3], attotime_never, 3);

	/* also one for each timer (heh) */
	state->tmr_timer[0] = timer_alloc(machine, timer_expire, 0);
	state->tmr_timer[1] = timer_alloc(machine, timer_expire, 0);
	state->tmr_timer[2] = timer_alloc(machine, timer_expire, 0);
	state->tmr_timer[3] = timer_alloc(machine, timer_expire, 0);
	timer_adjust_oneshot(state->tmr_timer[0], attotime_never, 0);
	timer_adjust_oneshot(state->tmr_timer[1], attotime_never, 1);
	timer_adjust_oneshot(state->tmr_timer[2], attotime_never, 2);
	timer_adjust_oneshot(state->tmr_timer[3], attotime_never, 3);

	/* and an IRQ handling timer */
	state->irq_timer = timer_alloc(machine, handle_irq, 0);
	timer_adjust_oneshot(state->irq_timer, attotime_never, 0);

	gba_video_start(machine);
}

ROM_START( gba )
	ROM_REGION( 0x8000, "bios", ROMREGION_ERASE00 )
	ROM_LOAD( "gba.bin", 0x000000, 0x004000, CRC(81977335) )

	/* cartridge region - 32 MBytes (128 Mbit) */
	ROM_REGION( 0x2000000, "cartridge", ROMREGION_ERASEFF )
ROM_END

static READ32_HANDLER( sram_r )
{
	gba_state *state = (gba_state *)space->machine->driver_data;

	return state->gba_sram[offset];
}

static WRITE32_HANDLER( sram_w )
{
	gba_state *state = (gba_state *)space->machine->driver_data;

	COMBINE_DATA(&state->gba_sram[offset]);
}

static READ32_HANDLER( flash64k_r )
{
	gba_state *state = (gba_state *)space->machine->driver_data;
	switch( state->flash64k_state )
	{
		case FLASH_IDLEBYTE0:
		case FLASH_IDLEBYTE1:
		case FLASH_IDLEBYTE2:
		case FLASH_ERASEBYTE0:
		case FLASH_ERASEBYTE1:
		case FLASH_ERASEBYTE2:
		        return state->gba_flash64k[offset];
		case FLASH_IDENT:
		        if( offset == 0 )
		        {
		        return ( state->gba_flash64k[0] & 0xffff0000 ) | 0x00001b32;
		        }
		        else
		        {
		        return state->gba_flash64k[offset];
		        }
		        return 0x00001b32;
		case FLASH_ERASE_ALL:
		case FLASH_ERASE_4K:
		        return state->gba_flash64k[offset];
	}
	return state->gba_flash64k[offset];
}

static WRITE32_HANDLER( flash64k_w )
{
	gba_state *state = (gba_state *)space->machine->driver_data;

	switch( state->flash64k_state )
	{
		case FLASH_IDLEBYTE0:
		case FLASH_ERASEBYTE0:
		        if( offset == 0x5555/4 && ~mem_mask == 0xffff00ff )
		        {
		        if( ( data & mem_mask ) == 0x0000aa00 )
		        {
		                if( state->flash64k_state == FLASH_IDLEBYTE0 )
		                {
			                state->flash64k_state = FLASH_IDLEBYTE1;
		                }
		                else if( state->flash64k_state == FLASH_ERASEBYTE0 )
		                {
			                state->flash64k_state = FLASH_ERASEBYTE1;
		                }
		        }
		        }
		        break;
		case FLASH_IDLEBYTE1:
		case FLASH_ERASEBYTE1:
		        if( offset == 0x2aaa/4 && ~mem_mask == 0xff00ffff )
		        {
		        if( ( data & mem_mask ) == 0x00550000 )
		        {
		                if( state->flash64k_state == FLASH_IDLEBYTE1 )
		                {
			                state->flash64k_state = FLASH_IDLEBYTE2;
		                }
		                else if( state->flash64k_state == FLASH_ERASEBYTE1 )
		                {
			                state->flash64k_state = FLASH_ERASEBYTE2;
		                }
		        }
		        }
		        break;
		case FLASH_IDLEBYTE2:
		case FLASH_ERASEBYTE2:
		        if( offset == 0x5555/4 && ~mem_mask == 0xffff00ff )
		        {
			        if( state->flash64k_state == FLASH_IDLEBYTE2 )
			        {
			                switch( ( data & mem_mask ) >> 8 )
			                {
			                case 0x80:
			                        state->flash64k_state = FLASH_ERASEBYTE0;
			                        break;
			                case 0x90:
			                        state->flash64k_state = FLASH_IDENT;
			                        break;
			                case 0xa0:
			                        state->flash64k_state = FLASH_WRITE;
			                        break;
			                }
			        }
			        else if( state->flash64k_state == FLASH_ERASEBYTE2 )
			        {
			                if( ( data & mem_mask ) == 0x00001000 )
			                {
				                UINT32 flashWord;
				                for( flashWord = 0; flashWord < 0x10000/4; flashWord++ )
				                {
				                        state->gba_flash64k[flashWord] = 0xffffffff;
				                }
				                state->flash64k_state = FLASH_ERASE_ALL;
			                }
			        }
		        }
		        else if( ( offset & 0xffffc3ff ) == 0 && ( data & mem_mask ) == 0x00000030 )
		        {
			        UINT32 flashWord;
			        state->flash64k_page = offset >> 10;
			        for( flashWord = offset; flashWord < offset + 0x1000/4; flashWord++ )
			        {
			                state->gba_flash64k[flashWord] = 0xffffffff;
			        }
			        state->flash64k_state = FLASH_ERASE_4K;
		        }
		        break;
		case FLASH_IDENT:
		        // Hack; any sensibly-written game should follow up with the relevant read, which will reset the state to FLASH_IDLEBYTE0.
		        state->flash64k_state = FLASH_IDLEBYTE0;
		        flash64k_w( space, offset, data, ~mem_mask );
		        break;
		case FLASH_ERASE_4K:
		        // Hack; any sensibly-written game should follow up with the relevant read, which will reset the state to FLASH_IDLEBYTE0.
		        state->flash64k_state = FLASH_IDLEBYTE0;
		        flash64k_w( space, offset, data, ~mem_mask );
		        break;
		case FLASH_ERASE_ALL:
		        // Hack; any sensibly-written game should follow up with the relevant read, which will reset the state to FLASH_IDLEBYTE0.
		        state->flash64k_state = FLASH_IDLEBYTE0;
		        flash64k_w( space, offset, data, ~mem_mask );
		        break;
		case FLASH_WRITE:
		        COMBINE_DATA(&state->gba_flash64k[offset]);
		        state->flash64k_state = FLASH_IDLEBYTE0;
		        break;
	}
}

static READ32_HANDLER( eeprom_r )
{
	UINT32 out;
	gba_state *state = (gba_state *)space->machine->driver_data;

	switch (state->eeprom_state)
	{
		case EEP_IDLE:
//          printf("eeprom_r: @ %x, mask %08x (state %d) (PC=%x) = %d\n", offset, ~mem_mask, state->eeprom_state, activecpu_get_pc(), 1);
			return 0x00010001;	// "ready"

        case EEP_READFIRST:
            state->eeprom_count--;

            if (!state->eeprom_count)
            {
                state->eeprom_count = 64;
                state->eeprom_bits = 0;
                state->eep_data = 0;
                state->eeprom_state = EEP_READ;
            }
            break;
		case EEP_READ:
			if ((state->eeprom_bits == 0) && (state->eeprom_count))
			{
				state->eep_data = state->gba_eeprom[state->eeprom_addr];
		       //       printf("EEPROM read @ %x = %x (%x)\n", state->eeprom_addr, state->eep_data, (state->eep_data & 0x80) ? 1 : 0);
				state->eeprom_addr++;
				state->eeprom_bits = 8;
			}

			out = (state->eep_data & 0x80) ? 1 : 0;
			out |= (out<<16);
			state->eep_data <<= 1;

			state->eeprom_bits--;
			state->eeprom_count--;

			if (!state->eeprom_count)
			{
				state->eeprom_state = EEP_IDLE;
			}

//          printf("out = %08x\n", out);
//          printf("eeprom_r: @ %x, mask %08x (state %d) (PC=%x) = %08x\n", offset, ~mem_mask, state->eeprom_state, activecpu_get_pc(), out);
			return out;
	}
//  printf("eeprom_r: @ %x, mask %08x (state %d) (PC=%x) = %d\n", offset, ~mem_mask, state->eeprom_state, activecpu_get_pc(), 0);
	return 0;
}

static WRITE32_HANDLER( eeprom_w )
{
	gba_state *state = (gba_state *)space->machine->driver_data;

	if (~mem_mask == 0x0000ffff)
	{
		data >>= 16;
	}

//  printf("eeprom_w: %x @ %x (state %d) (PC=%x)\n", data, offset, state->eeprom_state, activecpu_get_pc());

	switch (state->eeprom_state)
	{
		case EEP_IDLE:
			if (data == 1)
			{
				state->eeprom_state++;
			}
			break;

		case EEP_COMMAND:
			if (data == 1)
			{
                state->eeprom_command = EEP_READFIRST;
			}
			else
			{
				state->eeprom_command = EEP_WRITE;
			}
			state->eeprom_state = EEP_ADDR;
			state->eeprom_count = 6;
			state->eeprom_addr = 0;
			break;

		case EEP_ADDR:
			state->eeprom_addr <<= 1;
			state->eeprom_addr |= (data & 1);
			state->eeprom_count--;
			if (!state->eeprom_count)
			{
				state->eeprom_addr *= 8;	// each address points to 8 bytes
                if (state->eeprom_command == EEP_READFIRST)
				{
					state->eeprom_state = EEP_AFTERADDR;
				}
				else
				{
					state->eeprom_count = 64;
					state->eeprom_bits = 8;
					state->eeprom_state = EEP_WRITE;
					state->eep_data = 0;
				}
			}
			break;

		case EEP_AFTERADDR:
			state->eeprom_state = state->eeprom_command;
			state->eeprom_count = 64;
			state->eeprom_bits = 0;
			state->eep_data = 0;
            if( state->eeprom_state == EEP_READFIRST )
            {
                state->eeprom_count = 4;
            }
			break;

		case EEP_WRITE:
			state->eep_data<<= 1;
			state->eep_data |= (data & 1);
			state->eeprom_bits--;
			state->eeprom_count--;

			if (state->eeprom_bits == 0)
			{
				mame_printf_verbose("%08x: EEPROM: %02x to %x\n", cpu_get_pc(devtag_get_device(space->machine, "maincpu")), state->eep_data, state->eeprom_addr );
				state->gba_eeprom[state->eeprom_addr] = state->eep_data;
				state->eeprom_addr++;
				state->eep_data = 0;
				state->eeprom_bits = 8;
			}

			if (!state->eeprom_count)
			{
				state->eeprom_state = EEP_AFTERWRITE;
			}
			break;

		case EEP_AFTERWRITE:
			state->eeprom_state = EEP_IDLE;
			break;
	}
}

static DEVICE_IMAGE_LOAD( gba_cart )
{
	UINT8 *ROM = memory_region(image->machine, "cartridge");
	int i;
	gba_state *state = (gba_state *)image->machine->driver_data;

	nvsize = 0;
	nvptr = (UINT8 *)NULL;

	image_fread(image, ROM, image_length(image));

	for (i = 0; i < image_length(image); i++)
	{
		if (!memcmp(&ROM[i], "EEPROM_", 7))
		{
			nvptr = (UINT8 *)&state->gba_eeprom;
			nvsize = 0x2000;

			if (image_length(image) <= (16*1024*1024))
			{
				memory_install_read32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xd000000, 0xdffffff, 0, 0, eeprom_r);
				memory_install_write32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xd000000, 0xdffffff, 0, 0, eeprom_w);
			}
			else
			{
				memory_install_read32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xdffff00, 0xdffffff, 0, 0, eeprom_r);
				memory_install_write32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xdffff00, 0xdffffff, 0, 0, eeprom_w);
			}
			break;
		}
		else if (!memcmp(&ROM[i], "SRAM_", 5))
		{
			nvptr = (UINT8 *)&state->gba_sram;
			nvsize = 0x10000;

			memory_install_read32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xe000000, 0xe00ffff, 0, 0, sram_r);
			memory_install_write32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xe000000, 0xe00ffff, 0, 0, sram_w);
			break;
		}
		else if (!memcmp(&ROM[i], "FLASH1M_", 8))
		{
			mame_printf_verbose("game has 1M FLASH\n");
			break;
		}
		else if (!memcmp(&ROM[i], "FLASH", 5))
		{
			nvptr = (UINT8 *)&state->gba_flash64k;
			nvsize = 0x10000;

			memory_install_read32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xe000000, 0xe007fff, 0, 0, flash64k_r);
			memory_install_write32_handler(cpu_get_address_space(devtag_get_device(image->machine, "maincpu"), ADDRESS_SPACE_PROGRAM), 0xe000000, 0xe007fff, 0, 0, flash64k_w);
			break;
		}
		else if (!memcmp(&ROM[i], "SIIRTC_V", 8))
		{
			mame_printf_verbose("game has RTC\n");
			break;
		}
	}

	// if save media was found, reload it
	if (nvsize > 0)
	{
		image_battery_load(image, nvptr, nvsize);
		nvimage = image;
	}
	else
	{
		nvimage = NULL;
		nvsize = 0;
	}

	// mirror the ROM
	switch (image_length(image))
	{
		case 2*1024*1024:
			memcpy(ROM+0x200000, ROM, 0x200000);
		// intentional fall-through
		case 4*1024*1024:
			memcpy(ROM+0x400000, ROM, 0x400000);
		// intentional fall-through
		case 8*1024*1024:
			memcpy(ROM+0x800000, ROM, 0x800000);
		// intentional fall-through
		case 16*1024*1024:
			memcpy(ROM+0x1000000, ROM, 0x1000000);
			break;
	}

	return INIT_PASS;
}

static MACHINE_DRIVER_START( gbadv )
	MDRV_DRIVER_DATA(gba_state)

	MDRV_CPU_ADD("maincpu", ARM7, 16777216)
	MDRV_CPU_PROGRAM_MAP(gbadvance_map)

	MDRV_MACHINE_START(gba)
	MDRV_MACHINE_RESET(gba)

	MDRV_SCREEN_ADD("gbalcd", RASTER)	// htot hst vwid vtot vst vis
	MDRV_SCREEN_RAW_PARAMS(16777216/4, 308, 0,  240, 228, 0,  160)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_DEFAULT_LAYOUT(layout_lcd)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_PALETTE_INIT( gba )

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_STEREO("spkleft", "spkright")
	MDRV_SOUND_ADD("custom", GAMEBOY, 0)
	MDRV_SOUND_ROUTE(0, "spkleft", 0.50)
	MDRV_SOUND_ROUTE(1, "spkright", 0.50)
	MDRV_SOUND_ADD("direct_a_left", DAC, 0)			// GBA direct sound A left
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "spkleft", 0.50)
	MDRV_SOUND_ADD("direct_a_right", DAC, 0)		// GBA direct sound A right
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "spkright", 0.50)
	MDRV_SOUND_ADD("direct_b_left", DAC, 0)			// GBA direct sound B left
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "spkleft", 0.50)
	MDRV_SOUND_ADD("direct_b_right", DAC, 0)		// GBA direct sound B right
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "spkright", 0.50)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("gba,bin")
	MDRV_CARTSLOT_LOAD(gba_cart)
MACHINE_DRIVER_END

/* this emulates the GBA's hardware protection: the BIOS returns only zeros when the PC is not in it,
   and some games verify that as a protection check (notably Metroid Fusion) */
static DIRECT_UPDATE_HANDLER( gba_direct )
{
	if (address > 0x4000)
	{
		memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "bios")+0x4000);
	}
	else
	{
		memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "bios"));
	}

	return address;
}

static DRIVER_INIT(gbadv)
{
	memory_set_direct_update_handler( cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), gba_direct );
}

/*    YEAR  NAME PARENT COMPAT MACHINE INPUT   INIT   COMPANY     FULLNAME */
CONS( 2001, gba, 0,     0,     gbadv,  gbadv,  gbadv, "Nintendo", "Game Boy Advance", GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
