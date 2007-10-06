//============================================================
//
//  sdlsync.h - SDL core synchronization functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef __SDL_SYNC__
#define __SDL_SYNC__

#ifndef SDLMAME_WIN32

/***************************************************************************
    SYNCHRONIZATION INTERFACES - Events
***************************************************************************/

/* osd_event is an opaque type which represents a setable/resetable event */

typedef struct _osd_event osd_event;

/*-----------------------------------------------------------------------------
    osd_lock_event_alloc: allocate a new event

    Parameters:

        manualreset  - boolean. If true, the event will be automatically set
                       to non-signalled after a thread successfully waited for
                       it.
        initialstate - boolean. If true, the event is signalled initially.

    Return value:

        A pointer to the allocated event.
-----------------------------------------------------------------------------*/
osd_event *osd_event_alloc(int manualreset, int initialstate);

/*-----------------------------------------------------------------------------
    osd_event_wait: wait for an event to be signalled

    Parameters:

        event - The event to wait for. If the event is in signalled state, the
                function returns immediately. If not it will wait for the event
                to become signalled.
        timeout - timeout in osd_ticks

    Return value:

        TRUE:  The event was signalled
        FALSE: A timeout occurred 
-----------------------------------------------------------------------------*/
int osd_event_wait(osd_event *event, osd_ticks_t timeout);

//FIXME: description
/*-----------------------------------------------------------------------------
    osd_event_wait_multiple: wait for multiple events

    Parameters:

        event - The event to set to non-signalled state

    Return value:

	    None
-----------------------------------------------------------------------------*/

int osd_event_wait_multiple(int num, osd_event **events, osd_ticks_t timeout);

/*-----------------------------------------------------------------------------
    osd_event_reset: reset an event to non-signalled state

    Parameters:

        event - The event to set to non-signalled state

    Return value:

	    None
-----------------------------------------------------------------------------*/
void osd_event_reset(osd_event *event);

/*-----------------------------------------------------------------------------
    osd_event_reset: reset an event to non-signalled state

    Parameters:

        event - The event to set to non-signalled state

    Return value:

	    None
-----------------------------------------------------------------------------*/

int osd_event_wait_multiple(int num, osd_event *events[], osd_ticks_t timeout);

/*-----------------------------------------------------------------------------
    osd_event_set: set an event to signalled state

    Parameters:

        event - The event to set to signalled state

    Return value:

	    None
	    
	Notes:
	
	    All threads waiting for the event will be signalled.
-----------------------------------------------------------------------------*/
void osd_event_set(osd_event *event);


/*-----------------------------------------------------------------------------
    osd_event_free: free the memory and resources associated with an osd_event

    Parameters:

        event - a pointer to a previously allocated osd_event.

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_event_free(osd_event *event);


/***************************************************************************
    SYNCHRONIZATION INTERFACES - Threads
***************************************************************************/

/* osd_thread is an opaque type which represents a thread */
typedef struct _osd_thread osd_thread;

/* osd_thread_callback is a callback function that will be called from the thread */
typedef void *(*osd_thread_callback)(void *param);


/*-----------------------------------------------------------------------------
    osd_thread_create: create a new thread

    Parameters:

        callback - The callback function to be called once the thread is up
        cbparam  - The parameter to pass to the callback function.

    Return value:

        A pointer to the created thread.
-----------------------------------------------------------------------------*/
osd_thread *osd_thread_create(osd_thread_callback callback, void *cbparam);


/*-----------------------------------------------------------------------------
    osd_thread_adjust_priority: adjust priority of a thread

    Parameters:

        thread - A pointer to a previously created thread.
        adjust - signed integer to add to the thread priority

    Return value:

        TRUE on success, FALSE on failure
-----------------------------------------------------------------------------*/
int osd_thread_adjust_priority(osd_thread *thread, int adjust);


/*-----------------------------------------------------------------------------
    osd_thread_wait_free: wait for thread to finish and free resources

    Parameters:

        thread - A pointer to a previously created thread.

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_thread_wait_free(osd_thread *thread);


/*-----------------------------------------------------------------------------
    osd_num_processors: return the number of processors

    Parameters:

        None.

    Return value:

        Number of processors
-----------------------------------------------------------------------------*/
int osd_num_processors(void);


/*-----------------------------------------------------------------------------
    osd_atomic_lock: block other processes

    Parameters:

        None.

    Return value:

        None.
    
    Notes:
        This will be used on certain platforms to emulate atomic operations
        Please see osinclude.h 
-----------------------------------------------------------------------------*/
void osd_atomic_lock(void);


/*-----------------------------------------------------------------------------
    osd_atomic_unlock: unblock other processes

    Parameters:

        None.

    Return value:

        None.
    
    Notes:
        This will be used on certain platforms to emulate atomic operations
        Please see osinclude.h 
-----------------------------------------------------------------------------*/
void osd_atomic_unlock(void);


//============================================================
//  INLINE FUNCTIONS
//============================================================

#ifndef SDLMAME_WIN32
#ifdef X86_ASM

INLINE void * osd_atomic_cmpxchg(void * volatile *ptr, void *exchange, void *compare)
{
	long prev;
	__asm__ __volatile__("lock; cmpxchgl %1,%2"
						: "=a"(prev)
						: "r"((long) exchange), "m"(*ptr), "0"((long) compare)
						: "memory");
	return (void *) prev;
}

INLINE long osd_atomic_increment(long volatile *addend)
{
	__asm__ __volatile__(
		"lock;" "incl %0"
		:"=m" (*addend)
		:"m" (*addend));
	return *addend;
}


INLINE long osd_atomic_decrement(long volatile *addend)
{
	__asm__ __volatile__(
		"lock;" "decl %0"
		:"=m" (*addend)
		:"m" (*addend));
	return *addend;
}
#else

INLINE void * osd_atomic_cmpxchg(void * volatile *ptr, void *exchange, void *compare)
{
	void *ret = NULL;

	osd_atomic_lock();

#ifdef PTR64
	ret = (void*)(*((UINT64*)ptr));

	if ( *((UINT64*)ptr) == (UINT64)compare )
	{
		*((UINT64*)ptr) = (UINT64)exchange;
	}
#else
	ret = (void*)(*((UINT32*)ptr));

	if ( *((UINT32*)ptr) == (UINT32)compare )
	{
		*((UINT32*)ptr) = (UINT32)exchange;
	}
#endif

	osd_atomic_unlock();

	return ret;
}

INLINE long osd_atomic_increment(long volatile *addend)
{
	long ret;
	osd_atomic_lock();
	ret = (++(*addend));
	osd_atomic_unlock();
	return ret;
}


INLINE long osd_atomic_decrement(long volatile *addend)
{
	long ret;
	osd_atomic_lock();
	ret = (--(*addend));
	osd_atomic_unlock();
	return ret;
}
#endif

#else //SDLMAME_WIN32

INLINE PVOID osd_atomic_cmpxchg(PVOID volatile *ptr, PVOID exchange, PVOID compare)
{
	// the mingw headers don't put the volatile keyword on the first parameter
	// gcc also can't handle casting the result of a function
#ifdef __GNUC__
#ifdef PTR64
	UINT64 result = InterlockedCompareExchange64((UINT64)ptr, (UINT64)exchange, (UINT64)compare);
	return (PVOID)result;
#else
	LONG result = InterlockedCompareExchange((LPLONG)ptr, (LONG)exchange, (LONG)compare);
	return (PVOID)result;
#endif
#else
	return InterlockedCompareExchangePointer(ptr, exchange, compare);
#endif
}


INLINE LONG osd_atomic_increment(LONG volatile *addend)
{
	// the mingw headers don't put the volatile keyword on the first parameter
#ifdef __GNUC__
	return InterlockedIncrement((LPLONG)addend);
#else
	return InterlockedIncrement(addend);
#endif
}


INLINE LONG osd_atomic_decrement(LONG volatile *addend)
{
	// the mingw headers don't put the volatile keyword on the first parameter
#ifdef __GNUC__
	return InterlockedDecrement((LPLONG)addend);
#else
	return InterlockedDecrement(addend);
#endif
}

#endif

#endif	/* SDLMAME_WIN32 */
#endif	/* __SDL_SYNC__ */
