/***************************************************************************

    cpuexec.h

    Core multi-CPU execution engine.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __CPUEXEC_H__
#define __CPUEXEC_H__

#include "memory.h"
#include "timer.h"



/*************************************
 *
 *  CPU description for drivers
 *
 *************************************/

typedef struct _cpu_config cpu_config;
struct _cpu_config
{
	int			cpu_type;					/* index for the CPU type */
	int			cpu_flags;					/* flags; see #defines below */
	int			cpu_clock;					/* in Hertz */
	construct_map_t construct_map[ADDRESS_SPACES][2]; /* 2 memory maps per address space */
	void 		(*vblank_interrupt)(void);	/* for interrupts tied to VBLANK */
	int 		vblank_interrupts_per_frame;/* usually 1 */
	void 		(*timed_interrupt)(void);	/* for interrupts not tied to VBLANK */
	subseconds_t timed_interrupt_period;	/* period for periodic interrupts */
	void *		reset_param;				/* parameter for cpu_reset */
	const char *tag;
};



/*************************************
 *
 *  CPU flag constants
 *
 *************************************/

enum
{
	/* set this flag to disable execution of a CPU (if one is there for documentation */
	/* purposes only, for example */
	CPU_DISABLE = 0x0001
};




/*************************************
 *
 *  Core CPU execution
 *
 *************************************/

/* Prepare CPUs for execution */
void cpuexec_init(running_machine *machine);

/* Execute for a single timeslice */
void cpuexec_timeslice(void);



/*************************************
 *
 *  Optional watchdog
 *
 *************************************/

/* bang on the watchdog */
void watchdog_reset(void);

/* watchdog enabled when TRUE */
/* timer is set to reset state when going from disable to enable */
void watchdog_enable(int enable);



/*************************************
 *
 *  CPU scheduling
 *
 *************************************/

/* Suspension reasons */
enum
{
	SUSPEND_REASON_HALT 	= 0x0001,
	SUSPEND_REASON_RESET 	= 0x0002,
	SUSPEND_REASON_SPIN 	= 0x0004,
	SUSPEND_REASON_TRIGGER 	= 0x0008,
	SUSPEND_REASON_DISABLE 	= 0x0010,
	SUSPEND_ANY_REASON 		= ~0
};

/* Suspend the given CPU for a specific reason */
void cpunum_suspend(int cpunum, int reason, int eatcycles);

/* Suspend the given CPU for a specific reason */
void cpunum_resume(int cpunum, int reason);

/* Returns true if the given CPU is suspended for any of the given reasons */
int cpunum_is_suspended(int cpunum, int reason);

/* Aborts the timeslice for the active CPU */
void activecpu_abort_timeslice(void);

/* Returns the current local time for a CPU */
mame_time cpunum_get_localtime(int cpunum);

/* Returns the current CPU's unscaled running clock speed */
/* If you want to know the current effective running clock
 * after scaling it is just the double sec_to_cycles[cpunum] */
int cpunum_get_clock(int cpunum);

/* Sets the current CPU's clock speed and then adjusts for scaling */
void cpunum_set_clock(int cpunum, int clock);
void cpunum_set_clock_period(int cpunum, subseconds_t clock_period);

/* Returns the current scaling factor for a CPU's clock speed */
double cpunum_get_clockscale(int cpunum);

/* Sets the current scaling factor for a CPU's clock speed */
void cpunum_set_clockscale(int cpunum, double clockscale);

/* Temporarily boosts the interleave factor */
void cpu_boost_interleave(mame_time timeslice_time, mame_time boost_duration);



/*************************************
 *
 *  Timing helpers
 *
 *************************************/

/* Returns the number of cycles run so far this timeslice */
int cycles_currently_ran(void);

/* Returns the total number of CPU cycles */
UINT32 activecpu_gettotalcycles(void);
UINT64 activecpu_gettotalcycles64(void);

/* Returns the total number of CPU cycles for a given CPU */
UINT32 cpunum_gettotalcycles(int cpunum);
UINT64 cpunum_gettotalcycles64(int cpunum);

/* Safely eats cycles so we don't cross a timeslice boundary */
void activecpu_eat_cycles(int cycles);

/* Scales a given value by the ratio of fcount / fperiod */
int cpu_scalebyfcount(int value);



/*************************************
 *
 *  Video timing
 *
 *************************************/

/***** OBSOLETE NOTICE: these functions are no longer considered */
/***** to be the authority on scanline timing. Please use the */
/***** video_screen_* functions in video.c for newer drivers. */
/***** These functions may eventually go away. */

/* Recomputes the VBLANK timing after, e.g., a visible area change */
void cpu_compute_vblank_timing(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);



/*************************************
 *
 *  Synchronization
 *
 *************************************/

/* generate a trigger now */
void cpu_trigger(int trigger);

/* generate a trigger after a specific period of time */
void cpu_triggertime(mame_time duration, int trigger);

/* generate a trigger corresponding to an interrupt on the given CPU */
void cpu_triggerint(int cpunum);

/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger(int trigger);

/* burn specified CPU cycles until a timer trigger */
void cpunum_spinuntil_trigger(int cpunum, int trigger);

/* yield our timeslice until a timer trigger */
void cpu_yielduntil_trigger(int trigger);

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int(void);

/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int(void);

/* burn CPU cycles until our timeslice is up */
void cpu_spin(void);

/* yield our current timeslice */
void cpu_yield(void);

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time(mame_time duration);

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time(mame_time duration);



/*************************************
 *
 *  Core timing
 *
 *************************************/

/* Returns the number of times the interrupt handler will be called before
   the end of the current video frame. This is can be useful to interrupt
   handlers to synchronize their operation. If you call this from outside
   an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
   that the interrupt handler will be called once. */
int cpu_getiloops(void);


#endif	/* __CPUEXEC_H__ */
