/***************************************************************************

    timer.h

    Functions needed to generate timing and synchronization between several
    CPUs.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __TIMER_H__
#define __TIMER_H__

#include "mamecore.h"
#include <math.h>


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* subseconds are tracked in attosecond (10^-18) increments */
#define MAX_SUBSECONDS_SQRT			((subseconds_t)1000000000)
#define MAX_SUBSECONDS				(MAX_SUBSECONDS_SQRT * MAX_SUBSECONDS_SQRT)
#define MAX_SECONDS					((seconds_t)1000000000)



/***************************************************************************
    MACROS
***************************************************************************/

/* convert between a double and subseconds */
#define SUBSECONDS_TO_DOUBLE(x)		((double)(x) * (1.0 / (double)MAX_SUBSECONDS))
#define DOUBLE_TO_SUBSECONDS(x)		((subseconds_t)((x) * (double)MAX_SUBSECONDS))
#define USEC_TO_SUBSECONDS(us)		((subseconds_t)((us) * (MAX_SUBSECONDS / 1000000)))

/* hertz to subseconds macros */
#define SUBSECONDS_TO_HZ(x)			((double)MAX_SUBSECONDS / (double)(x))
#define HZ_TO_SUBSECONDS(x)			(MAX_SUBSECONDS / (x))

/* convert cycles on a given CPU to/from mame_time */
#define MAME_TIME_TO_CYCLES(cpu,t)	((t).seconds * cycles_per_second[cpu] + (t).subseconds / subseconds_per_cycle[cpu])
#define MAME_TIME_IN_CYCLES(c,cpu)	(make_mame_time((c) / cycles_per_second[cpu], (c) * subseconds_per_cycle[cpu]))

/* useful macros for describing mame_times */
#define MAME_TIME_IN_HZ(hz)		make_mame_time(0, MAX_SUBSECONDS / (hz))
#define MAME_TIME_IN_SEC(s)		make_mame_time((s)  / 1, (s) % 1 /* mod here errors on doubles, which is intended */)
#define MAME_TIME_IN_MSEC(ms)	make_mame_time((ms) / 1000, ((ms) % 1000) * (MAX_SUBSECONDS / 1000))
#define MAME_TIME_IN_USEC(us)	make_mame_time((us) / 1000000, ((us) % 1000000) * (MAX_SUBSECONDS / 1000000))
#define MAME_TIME_IN_NSEC(ns)	make_mame_time((ns) / 1000000000, ((ns) % 1000000000) * (MAX_SUBSECONDS / 1000000000))

/* macro for the RC time constant on a 74LS123 with C > 1000pF */
/* R is in ohms, C is in farads */
#define TIME_OF_74LS123(r,c)			(0.45 * (double)(r) * (double)(c))

/* macros for the RC time constant on a 555 timer IC */
/* R is in ohms, C is in farads */
#define PERIOD_OF_555_MONOSTABLE(r,c)	MAME_TIME_IN_NSEC((subseconds_t)(1100000000 * (double)(r) * (double)(c)))
#define PERIOD_OF_555_ASTABLE(r1,r2,c)	MAME_TIME_IN_NSEC((subseconds_t)( 693000000 * ((double)(r1) + 2.0 * (double)(r2)) * (double)(c)))

/* macros that map all allocations to provide file/line/functions to the callee */
#define mame_timer_alloc(c)				_mame_timer_alloc(c, __FILE__, __LINE__, #c)
#define mame_timer_alloc_ptr(c,p)		_mame_timer_alloc_ptr(c, p, __FILE__, __LINE__, #c)
#define mame_timer_pulse(e,p,c)			_mame_timer_pulse(e, p, c, __FILE__, __LINE__, #c)
#define mame_timer_pulse_ptr(e,p,c)		_mame_timer_pulse_ptr(e, p, c, __FILE__, __LINE__, #c)
#define mame_timer_set(d,p,c)			_mame_timer_set(d, p, c, __FILE__, __LINE__, #c)
#define mame_timer_set_ptr(d,p,c)		_mame_timer_set_ptr(d, p, c, __FILE__, __LINE__, #c)

/* macros for a timer callback functions */
#define TIMER_CALLBACK(name)			void name(running_machine *machine, int param)
#define TIMER_CALLBACK_PTR(name)		void name(running_machine *machine, void *param)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* opaque type for representing a timer */
typedef struct _mame_timer mame_timer;

/* these core types describe a 96-bit time value */
typedef INT64 subseconds_t;
typedef INT32 seconds_t;

typedef struct _mame_time mame_time;
struct _mame_time
{
	seconds_t		seconds;
	subseconds_t	subseconds;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* constant times for zero and never */
extern mame_time time_zero;
extern mame_time time_never;

/* arrays containing mappings between CPU cycle times and timer values */
extern subseconds_t subseconds_per_cycle[];
extern UINT32 cycles_per_second[];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void timer_init(running_machine *machine);
void timer_destructor(void *ptr, size_t size);
int timer_count_anonymous(void);

mame_time mame_timer_next_fire_time(void);
void mame_timer_set_global_time(mame_time newbase);
mame_timer *_mame_timer_alloc(void (*callback)(running_machine *, int), const char *file, int line, const char *func);
mame_timer *_mame_timer_alloc_ptr(void (*callback)(running_machine *, void *), void *param, const char *file, int line, const char *func);
void mame_timer_adjust(mame_timer *which, mame_time duration, INT32 param, mame_time period);
void mame_timer_adjust_ptr(mame_timer *which, mame_time duration, mame_time period);
void _mame_timer_pulse(mame_time period, INT32 param, void (*callback)(running_machine *, int), const char *file, int line, const char *func);
void _mame_timer_pulse_ptr(mame_time period, void *param, void (*callback)(running_machine *, void *), const char *file, int line, const char *func);
void _mame_timer_set(mame_time duration, INT32 param, void (*callback)(running_machine *, int), const char *file, int line, const char *func);
void _mame_timer_set_ptr(mame_time duration, void *param, void (*callback)(running_machine *, void *), const char *file, int line, const char *func);
void mame_timer_reset(mame_timer *which, mame_time duration);
int mame_timer_enable(mame_timer *which, int enable);
int mame_timer_enabled(mame_timer *which);
int mame_timer_get_param(mame_timer *which);
void *mame_timer_get_param_ptr(mame_timer *which);
mame_time mame_timer_timeelapsed(mame_timer *which);
mame_time mame_timer_timeleft(mame_timer *which);
mame_time mame_timer_get_time(void);
mame_time mame_timer_starttime(mame_timer *which);
mame_time mame_timer_firetime(mame_timer *which);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    make_mame_time - assemble a mame_time from
    seconds and subseconds components
-------------------------------------------------*/

INLINE mame_time make_mame_time(seconds_t _secs, subseconds_t _subsecs)
{
	mame_time result;
	result.seconds = _secs;
	result.subseconds = _subsecs;
	return result;
}


/*-------------------------------------------------
    mame_time_to_subseconds - convert a mame_time
    to subseconds, clamping to maximum positive/
    negative values
-------------------------------------------------*/

INLINE subseconds_t mame_time_to_subseconds(mame_time _time)
{
	/* positive values between 0 and 1 second */
	if (_time.seconds == 0)
		return _time.subseconds;

	/* negative values between -1 and 0 seconds */
	else if (_time.seconds == -1)
		return _time.subseconds - MAX_SUBSECONDS;

	/* out-of-range positive values */
	else if (_time.seconds > 0)
		return MAX_SUBSECONDS;

	/* out-of-range negative values */
	else
		return -MAX_SUBSECONDS;
}


/*-------------------------------------------------
    mame_time_to_double - convert from mame_time
    to double
-------------------------------------------------*/

INLINE double mame_time_to_double(mame_time _time)
{
	return (double)_time.seconds + SUBSECONDS_TO_DOUBLE(_time.subseconds);
}


/*-------------------------------------------------
    double_to_mame_time - convert from double to
    mame_time
-------------------------------------------------*/

INLINE mame_time double_to_mame_time(double _time)
{
	mame_time abstime;

	/* set seconds to the integral part */
	abstime.seconds = floor(_time);

	/* set subseconds to the fractional part */
	_time -= (double)abstime.seconds;
	abstime.subseconds = DOUBLE_TO_SUBSECONDS(_time);
	return abstime;
}


/*-------------------------------------------------
    add_mame_times - add two mame_times
-------------------------------------------------*/

INLINE mame_time add_mame_times(mame_time _time1, mame_time _time2)
{
	mame_time result;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS || _time2.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds + _time2.subseconds;
	result.seconds = _time1.seconds + _time2.seconds;

	/* normalize and return */
	if (result.subseconds >= MAX_SUBSECONDS)
	{
		result.subseconds -= MAX_SUBSECONDS;
		result.seconds++;
	}
	return result;
}


/*-------------------------------------------------
    add_subseconds_to_mame_time - add subseconds
    to a mame_time
-------------------------------------------------*/

INLINE mame_time add_subseconds_to_mame_time(mame_time _time1, subseconds_t _subseconds)
{
	mame_time result;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds + _subseconds;
	result.seconds = _time1.seconds;

	/* normalize and return */
	if (result.subseconds >= MAX_SUBSECONDS)
	{
		result.subseconds -= MAX_SUBSECONDS;
		result.seconds++;
	}
	return result;
}


/*-------------------------------------------------
    sub_mame_times - subtract two mame_times
-------------------------------------------------*/

INLINE mame_time sub_mame_times(mame_time _time1, mame_time _time2)
{
	mame_time result;

	/* if time1 is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds - _time2.subseconds;
	result.seconds = _time1.seconds - _time2.seconds;

	/* normalize and return */
	if (result.subseconds < 0)
	{
		result.subseconds += MAX_SUBSECONDS;
		result.seconds--;
	}
	return result;
}


/*-------------------------------------------------
    sub_subseconds_from_mame_time - subtract
    subseconds from a mame_time
-------------------------------------------------*/

INLINE mame_time sub_subseconds_from_mame_time(mame_time _time1, subseconds_t _subseconds)
{
	mame_time result;

	/* if time1 is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds - _subseconds;
	result.seconds = _time1.seconds;

	/* normalize and return */
	if (result.subseconds < 0)
	{
		result.subseconds += MAX_SUBSECONDS;
		result.seconds--;
	}
	return result;
}


/*-------------------------------------------------
    scale_up_mame_time - multiply a mame_time by
    a constant
-------------------------------------------------*/

INLINE mame_time scale_up_mame_time(mame_time _time1, UINT32 factor)
{
	UINT32 subseclo, subsechi;
	UINT64 templo, temphi;
	mame_time result;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* 0 times anything is zero */
	if (factor == 0)
		return time_zero;

	/* break subseconds into lower and upper halves */
	subseclo = (UINT32)(_time1.subseconds % MAX_SUBSECONDS_SQRT);
	subsechi = (UINT32)(_time1.subseconds / MAX_SUBSECONDS_SQRT);

	/* get a 64-bit product for each one */
	templo = (UINT64)subseclo * (UINT64)factor;
	temphi = (UINT64)subsechi * (UINT64)factor;

	/* separate out the low and carry into the high */
	subseclo = templo % MAX_SUBSECONDS_SQRT;
	temphi += templo / MAX_SUBSECONDS_SQRT;
	subsechi = temphi % MAX_SUBSECONDS_SQRT;

	/* assemble the two parts into final subseconds and carry into seconds */
	result.subseconds = (subseconds_t)subseclo + MAX_SUBSECONDS_SQRT * (subseconds_t)subsechi;
	result.seconds = _time1.seconds * factor + temphi / MAX_SUBSECONDS_SQRT;

	/* max out at never time */
	if (result.seconds >= MAX_SECONDS)
		return time_never;

	return result;
}


/*-------------------------------------------------
    scale_down_mame_time - divide a mame_time by
    a constant
-------------------------------------------------*/

INLINE mame_time scale_down_mame_time(mame_time _time1, UINT32 factor)
{
	mame_time result;
	int shift_count;
	int carry;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	result.seconds = _time1.seconds / factor;
	carry = _time1.seconds - (result.seconds * factor);

	/* shift the subseconds to the right by log2 of the factor, so it won't overflow */
	shift_count = (log(carry) / log(2)) + 1;
	result.subseconds = (((carry * (MAX_SUBSECONDS >> shift_count)) + (_time1.subseconds >> shift_count)) / factor) << shift_count;

	return result;
}


/*-------------------------------------------------
    compare_mame_times - compare two mame_times
-------------------------------------------------*/

INLINE int compare_mame_times(mame_time _time1, mame_time _time2)
{
	if (_time1.seconds > _time2.seconds)
		return 1;
	if (_time1.seconds < _time2.seconds)
		return -1;
	if (_time1.subseconds > _time2.subseconds)
		return 1;
	if (_time1.subseconds < _time2.subseconds)
		return -1;
	return 0;
}


/*-------------------------------------------------
    timer_call_after_resynch - synchs the CPUs
    and calls the callback immediately
-------------------------------------------------*/

INLINE void timer_call_after_resynch(INT32 param, void (*callback)(running_machine *, int))
{
	mame_timer_set(time_zero, param, callback);
}


INLINE void timer_call_after_resynch_ptr(void *param, void (*callback)(running_machine *, void *))
{
	mame_timer_set_ptr(time_zero, param, callback);
}


#endif	/* __TIMER_H__ */
