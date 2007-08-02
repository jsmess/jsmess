/***************************************************************************

    timer.c

    Functions needed to generate timing and synchronization between several
    CPUs.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "profiler.h"
#include "pool.h"
#include <math.h>


/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE 0

#if VERBOSE
#define LOG(x)			do { logerror x; } while (0)
#else
#define LOG(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_TIMERS		256



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* in timer.h: typedef struct _mame_timer mame_timer; */
struct _mame_timer
{
	mame_timer *	next;
	mame_timer *	prev;
	void 			(*callback)(running_machine *, int);
	void			(*callback_ptr)(running_machine *, void *);
	int 			callback_param;
	void *			callback_ptr_param;
	const char *	file;
	int 			line;
	const char *	func;
	UINT8 			enabled;
	UINT8 			temporary;
	UINT8			ptr;
	mame_time 		period;
	mame_time 		start;
	mame_time 		expire;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* conversion constants */
subseconds_t subseconds_per_cycle[MAX_CPU];
UINT32 cycles_per_second[MAX_CPU];

/* list of active timers */
static mame_timer timers[MAX_TIMERS];
static mame_timer *timer_head;
static mame_timer *timer_free_head;
static mame_timer *timer_free_tail;

/* other internal states */
static mame_time global_basetime;
static mame_timer *callback_timer;
static int callback_timer_modified;
static mame_time callback_timer_expire_time;

/* other constant times */
mame_time time_zero;
mame_time time_never;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void timer_postload(void);
static void timer_logtimers(void);
static void timer_remove(mame_timer *which);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_current_time - return the current time
-------------------------------------------------*/

INLINE mame_time get_current_time(void)
{
	int activecpu;

	/* if we're currently in a callback, use the timer's expiration time as a base */
	if (callback_timer != NULL)
		return callback_timer_expire_time;

	/* if we're executing as a particular CPU, use its local time as a base */
	activecpu = cpu_getactivecpu();
	if (activecpu >= 0)
		return cpunum_get_localtime(activecpu);

	/* otherwise, return the current global base time */
	return global_basetime;
}


/*-------------------------------------------------
    timer_new - allocate a new timer
-------------------------------------------------*/

INLINE mame_timer *timer_new(void)
{
	mame_timer *timer;

	/* remove an empty entry */
	if (!timer_free_head)
	{
		timer_logtimers();
		fatalerror("Out of timers!");
	}
	timer = timer_free_head;
	timer_free_head = timer->next;
	if (!timer_free_head)
		timer_free_tail = NULL;

	return timer;
}


/*-------------------------------------------------
    timer_list_insert - insert a new timer into
    the list at the appropriate location
-------------------------------------------------*/

INLINE void timer_list_insert(mame_timer *timer)
{
	mame_time expire = timer->enabled ? timer->expire : time_never;
	mame_timer *t, *lt = NULL;

	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		int tnum = 0;

		/* loop over the timer list */
		for (t = timer_head; t; t = t->next, tnum++)
		{
			if (t == timer)
				fatalerror("This timer is already inserted in the list!");
			if (tnum == MAX_TIMERS-1)
				fatalerror("Timer list is full!");
		}
	}
	#endif

	/* loop over the timer list */
	for (t = timer_head; t; lt = t, t = t->next)
	{
		/* if the current list entry expires after us, we should be inserted before it */
		if (compare_mame_times(t->expire, expire) > 0)
		{
			/* link the new guy in before the current list entry */
			timer->prev = t->prev;
			timer->next = t;

			if (t->prev)
				t->prev->next = timer;
			else
				timer_head = timer;
			t->prev = timer;
			return;
		}
	}

	/* need to insert after the last one */
	if (lt)
		lt->next = timer;
	else
		timer_head = timer;
	timer->prev = lt;
	timer->next = NULL;
}


/*-------------------------------------------------
    timer_list_remove - remove a timer from the
    linked list
-------------------------------------------------*/

INLINE void timer_list_remove(mame_timer *timer)
{
	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		mame_timer *t;

		/* loop over the timer list */
		for (t = timer_head; t && t != timer; t = t->next) ;
		if (t == NULL)
			fatalerror("timer (%s from %s:%d) not found in list", timer->func, timer->file, timer->line);
	}
	#endif

	/* remove it from the list */
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
}



/***************************************************************************
    INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    timer_init - initialize the timer system
-------------------------------------------------*/

void timer_init(running_machine *machine)
{
	int i;

	/* init the constant times */
	time_zero.seconds = time_zero.subseconds = 0;
	time_never.seconds = MAX_SECONDS;
	time_never.subseconds = MAX_SUBSECONDS - 1;

	/* we need to wait until the first call to timer_cyclestorun before using real CPU times */
	global_basetime = time_zero;
	callback_timer = NULL;
	callback_timer_modified = FALSE;

	/* register with the save state system */
	state_save_push_tag(0);
	state_save_register_item("timer", 0, global_basetime.seconds);
	state_save_register_item("timer", 0, global_basetime.subseconds);
	state_save_register_func_postload(timer_postload);
	state_save_pop_tag();

	/* reset the timers */
	memset(timers, 0, sizeof(timers));

	/* initialize the lists */
	timer_head = NULL;
	timer_free_head = &timers[0];
	for (i = 0; i < MAX_TIMERS-1; i++)
		timers[i].next = &timers[i+1];
	timers[MAX_TIMERS-1].next = NULL;
	timer_free_tail = &timers[MAX_TIMERS-1];
}


/*-------------------------------------------------
    timer_destructor - destruct a timer from a
    pool callback
-------------------------------------------------*/

void timer_destructor(void *ptr, size_t size)
{
	timer_remove(ptr);
}



/***************************************************************************
    SCHEDULING HELPERS
***************************************************************************/

/*-------------------------------------------------
    mame_timer_next_fire_time - return the
    time when the next timer will fire
-------------------------------------------------*/

mame_time mame_timer_next_fire_time(void)
{
	return timer_head->expire;
}


/*-------------------------------------------------
    timer_adjust_global_time - adjust the global
    time; this is also where we fire the timers
-------------------------------------------------*/

void mame_timer_set_global_time(mame_time newbase)
{
	mame_timer *timer;

	/* set the new global offset */
	global_basetime = newbase;

	LOG(("mame_timer_set_global_time: new=%.9f head->expire=%.9f\n", mame_time_to_double(newbase), mame_time_to_double(timer_head->expire)));

	/* now process any timers that are overdue */
	while (compare_mame_times(timer_head->expire, global_basetime) <= 0)
	{
		int was_enabled = timer_head->enabled;

		/* if this is a one-shot timer, disable it now */
		timer = timer_head;
		if (compare_mame_times(timer->period, time_zero) == 0 || compare_mame_times(timer->period, time_never) == 0)
			timer->enabled = FALSE;

		/* set the global state of which callback we're in */
		callback_timer_modified = FALSE;
		callback_timer = timer;
		callback_timer_expire_time = timer->expire;

		/* call the callback */
		if (was_enabled)
		{
			if (!timer->ptr && timer->callback)
			{
				LOG(("Timer %s:%d[%s] fired (expire=%.9f)\n", timer->file, timer->line, timer->func, mame_time_to_double(timer->expire)));
				profiler_mark(PROFILER_TIMER_CALLBACK);
				(*timer->callback)(Machine, timer->callback_param);
				profiler_mark(PROFILER_END);
			}
			else if (timer->ptr && timer->callback_ptr)
			{
				LOG(("Timer %s:%d[%s] fired (expire=%.9f)\n", timer->file, timer->line, timer->func, mame_time_to_double(timer->expire)));
				profiler_mark(PROFILER_TIMER_CALLBACK);
				(*timer->callback_ptr)(Machine, timer->callback_ptr_param);
				profiler_mark(PROFILER_END);
			}
		}

		/* clear the callback timer global */
		callback_timer = NULL;

		/* reset or remove the timer, but only if it wasn't modified during the callback */
		if (!callback_timer_modified)
		{
			/* if the timer is temporary, remove it now */
			if (timer->temporary)
				timer_remove(timer);

			/* otherwise, reschedule it */
			else
			{
				timer->start = timer->expire;
				timer->expire = add_mame_times(timer->expire, timer->period);

				timer_list_remove(timer);
				timer_list_insert(timer);
			}
		}
	}
}



/***************************************************************************
    SAVE/RESTORE HELPERS
***************************************************************************/

/*-------------------------------------------------
    timer_register_save - register ourself with
    the save state system
-------------------------------------------------*/

static void timer_register_save(mame_timer *timer)
{
	char buf[256];
	int count = 0;
	mame_timer *t;

	/* find other timers that match our func name */
	for (t = timer_head; t; t = t->next)
		if (!strcmp(t->func, timer->func))
			count++;

	/* make up a name */
	sprintf(buf, "timer.%s", timer->func);

	/* use different instances to differentiate the bits */
	state_save_push_tag(0);
	state_save_register_item(buf, count, timer->callback_param);
	state_save_register_item(buf, count, timer->enabled);
	state_save_register_item(buf, count, timer->period.seconds);
	state_save_register_item(buf, count, timer->period.subseconds);
	state_save_register_item(buf, count, timer->start.seconds);
	state_save_register_item(buf, count, timer->start.subseconds);
	state_save_register_item(buf, count, timer->expire.seconds);
	state_save_register_item(buf, count, timer->expire.subseconds);
	state_save_pop_tag();
}


/*-------------------------------------------------
    timer_postload - after loading a save state
-------------------------------------------------*/

static void timer_postload(void)
{
	mame_timer *privlist = NULL;
	mame_timer *t;

	/* remove all timers and make a private list */
	while (timer_head)
	{
		t = timer_head;

		/* temporary timers go away entirely */
		if (t->temporary)
			timer_remove(t);

		/* permanent ones get added to our private list */
		else
		{
			timer_list_remove(t);
			t->next = privlist;
			privlist = t;
		}
	}

	/* now add them all back in; this effectively re-sorts them by time */
	while (privlist)
	{
		t = privlist;
		privlist = t->next;
		timer_list_insert(t);
	}
}


/*-------------------------------------------------
    timer_count_anonymous - count the number of
    anonymous (non-saveable) timers
-------------------------------------------------*/

int timer_count_anonymous(void)
{
	mame_timer *t;
	int count = 0;

	logerror("timer_count_anonymous:\n");
	for (t = timer_head; t; t = t->next)
		if (t->temporary && t != callback_timer)
		{
			count++;
			logerror("  Temp. timer %p, file %s:%d[%s]\n", (void *) t, t->file, t->line, t->func);
		}
	logerror("%d temporary timers found\n", count);

	return count;
}



/***************************************************************************
    CORE TIMER ALLOCATION
***************************************************************************/

/*-------------------------------------------------
    timer_alloc - allocate a permament timer that
    isn't primed yet
-------------------------------------------------*/

INLINE mame_timer *_mame_timer_alloc_common(void (*callback)(running_machine *, int), void (*callback_ptr)(running_machine *, void *), void *param, const char *file, int line, const char *func, int temp)
{
	mame_time time = get_current_time();
	mame_timer *timer = timer_new();

	/* fill in the record */
	timer->callback = callback;
	timer->callback_ptr = callback_ptr;
	timer->callback_param = 0;
	timer->callback_ptr_param = param;
	timer->enabled = FALSE;
	timer->temporary = temp;
	timer->ptr = (callback_ptr != NULL);
	timer->period = time_zero;
	timer->file = file;
	timer->line = line;
	timer->func = func;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = time_never;
	timer_list_insert(timer);

	/* if we're not temporary, register ourselve with the save state system */
	if (!temp)
	{
		timer_register_save(timer);
		restrack_register_object(OBJTYPE_TIMER, timer, 0, file, line);
	}

	/* return a handle */
	return timer;
}

mame_timer *_mame_timer_alloc(void (*callback)(running_machine *, int), const char *file, int line, const char *func)
{
	return _mame_timer_alloc_common(callback, NULL, NULL, file, line, func, FALSE);
}

mame_timer *_mame_timer_alloc_ptr(void (*callback_ptr)(running_machine *, void *), void *param, const char *file, int line, const char *func)
{
	return _mame_timer_alloc_common(NULL, callback_ptr, param, file, line, func, FALSE);
}


/*-------------------------------------------------
    timer_remove - remove a timer from the
    system
-------------------------------------------------*/

static void timer_remove(mame_timer *which)
{
	/* if this is a callback timer, note that */
	if (which == callback_timer)
		callback_timer_modified = TRUE;

	/* remove it from the list */
	timer_list_remove(which);

	/* free it up by adding it back to the free list */
	if (timer_free_tail)
		timer_free_tail->next = which;
	else
		timer_free_head = which;
	which->next = NULL;
	timer_free_tail = which;
}



/***************************************************************************
    CORE TIMER ADJUSTMENT
***************************************************************************/

/*-------------------------------------------------
    timer_adjust - adjust the time when this
    timer will fire, and whether or not it will
    fire periodically
-------------------------------------------------*/

INLINE void mame_timer_adjust_common(mame_timer *which, mame_time duration, INT32 param, mame_time period)
{
	mame_time time = get_current_time();

	/* if this is the callback timer, mark it modified */
	if (which == callback_timer)
		callback_timer_modified = TRUE;

	/* compute the time of the next firing and insert into the list */
	which->callback_param = param;
	which->enabled = TRUE;

	/* clamp negative times to 0 */
	if (duration.seconds < 0)
		duration = time_zero;

	/* set the start and expire times */
	which->start = time;
	which->expire = add_mame_times(time, duration);
	which->period = period;

	/* remove and re-insert the timer in its new order */
	timer_list_remove(which);
	timer_list_insert(which);

	/* if this was inserted as the head, abort the current timeslice and resync */
	LOG(("timer_adjust %s.%s:%d to expire @ %.9f\n", which->file, which->func, which->line, mame_time_to_double(which->expire)));
	if (which == timer_head && cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}

void mame_timer_adjust(mame_timer *which, mame_time duration, INT32 param, mame_time period)
{
	if (which->ptr)
		fatalerror("mame_timer_adjust called on a ptr timer!");
	mame_timer_adjust_common(which, duration, param, period);
}

void mame_timer_adjust_ptr(mame_timer *which, mame_time duration, mame_time period)
{
	if (!which->ptr)
		fatalerror("mame_timer_adjust_ptr called on a non-ptr timer!");
	mame_timer_adjust_common(which, duration, 0, period);
}



/***************************************************************************
    SIMPLIFIED ANONYMOUS TIMER MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    timer_pulse - allocate a pulse timer, which
    repeatedly calls the callback using the given
    period
-------------------------------------------------*/

void _mame_timer_pulse(mame_time period, INT32 param, void (*callback)(running_machine *, int), const char *file, int line, const char *func)
{
	mame_timer *timer = _mame_timer_alloc_common(callback, NULL, NULL, file, line, func, FALSE);

	/* adjust to our liking */
	mame_timer_adjust(timer, period, param, period);
}

void _mame_timer_pulse_ptr(mame_time period, void *param, void (*callback)(running_machine *, void *), const char *file, int line, const char *func)
{
	mame_timer *timer = _mame_timer_alloc_common(NULL, callback, param, file, line, func, FALSE);

	/* adjust to our liking */
	mame_timer_adjust_ptr(timer, period, period);
}


/*-------------------------------------------------
    timer_set - allocate a one-shot timer, which
    calls the callback after the given duration
-------------------------------------------------*/

void _mame_timer_set(mame_time duration, INT32 param, void (*callback)(running_machine *, int), const char *file, int line, const char *func)
{
	mame_timer *timer = _mame_timer_alloc_common(callback, NULL, NULL, file, line, func, TRUE);

	/* adjust to our liking */
	mame_timer_adjust(timer, duration, param, time_zero);
}

void _mame_timer_set_ptr(mame_time duration, void *param, void (*callback)(running_machine *, void *), const char *file, int line, const char *func)
{
	mame_timer *timer = _mame_timer_alloc_common(NULL, callback, param, file, line, func, TRUE);

	/* adjust to our liking */
	mame_timer_adjust_ptr(timer, duration, time_zero);
}



/***************************************************************************
    MISCELLANEOUS CONTROLS
***************************************************************************/

/*-------------------------------------------------
    timer_reset - reset the timing on a timer
-------------------------------------------------*/

void mame_timer_reset(mame_timer *which, mame_time duration)
{
	/* adjust the timer */
	if (!which->ptr)
		mame_timer_adjust(which, duration, which->callback_param, which->period);
	else
		mame_timer_adjust_ptr(which, duration, which->period);
}


/*-------------------------------------------------
    timer_enable - enable/disable a timer
-------------------------------------------------*/

int mame_timer_enable(mame_timer *which, int enable)
{
	int old;

	/* set the enable flag */
	old = which->enabled;
	which->enabled = enable;

	/* remove the timer and insert back into the list */
	timer_list_remove(which);
	timer_list_insert(which);

	return old;
}


/*-------------------------------------------------
    timer_enabled - determine if a timer is
    enabled
-------------------------------------------------*/

int mame_timer_enabled(mame_timer *which)
{
	return which->enabled;
}


/*-------------------------------------------------
    timer_get_param
    timer_get_param_ptr - returns the callback
    parameter of a timer
-------------------------------------------------*/

int mame_timer_get_param(mame_timer *which)
{
	return which->callback_param;
}


void *mame_timer_get_param_ptr(mame_timer *which)
{
	return which->callback_ptr_param;
}


/***************************************************************************
    TIMING FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    timer_timeelapsed - return the time since the
    last trigger
-------------------------------------------------*/

mame_time mame_timer_timeelapsed(mame_timer *which)
{
	return sub_mame_times(get_current_time(), which->start);
}


/*-------------------------------------------------
    timer_timeleft - return the time until the
    next trigger
-------------------------------------------------*/

mame_time mame_timer_timeleft(mame_timer *which)
{
	return sub_mame_times(which->expire, get_current_time());
}


/*-------------------------------------------------
    timer_get_time - return the current time
-------------------------------------------------*/

mame_time mame_timer_get_time(void)
{
	return get_current_time();
}


/*-------------------------------------------------
    timer_starttime - return the time when this
    timer started counting
-------------------------------------------------*/

mame_time mame_timer_starttime(mame_timer *which)
{
	return which->start;
}


/*-------------------------------------------------
    timer_firetime - return the time when this
    timer will fire next
-------------------------------------------------*/

mame_time mame_timer_firetime(mame_timer *which)
{
	return which->expire;
}



/***************************************************************************
    DEBUGGING
***************************************************************************/

/*-------------------------------------------------
    timer_logtimers - log all the timers
-------------------------------------------------*/

static void timer_logtimers(void)
{
	mame_timer *t;

	logerror("===============\n");
	logerror("TIMER LOG START\n");
	logerror("===============\n");

	logerror("Enqueued timers:\n");
	for (t = timer_head; t; t = t->next)
		logerror("  Start=%15.6f Exp=%15.6f Per=%15.6f Ena=%d Tmp=%d (%s:%d[%s])\n",
			mame_time_to_double(t->start), mame_time_to_double(t->expire), mame_time_to_double(t->period), t->enabled, t->temporary, t->file, t->line, t->func);

	logerror("Free timers:\n");
	for (t = timer_free_head; t; t = t->next)
		logerror("  Start=%15.6f Exp=%15.6f Per=%15.6f Ena=%d Tmp=%d (%s:%d[%s])\n",
			mame_time_to_double(t->start), mame_time_to_double(t->expire), mame_time_to_double(t->period), t->enabled, t->temporary, t->file, t->line, t->func);

	logerror("==============\n");
	logerror("TIMER LOG STOP\n");
	logerror("==============\n");
}
