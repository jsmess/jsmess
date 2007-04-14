//============================================================
//
//  osdefs.h - System-specific defines
//
//  Copyright (c) 2007 Albert Lee.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef _OSDEFS_H_
#define _OSDEFS_H_


#ifdef __APPLE__
#define SDLMAME_DARWIN 1
#endif /* __APPLE__ */

#ifdef SDLMAME_UNIX

#if defined(__sun__) && defined(__svr4__)
#define SDLMAME_SOLARIS 1

#elif defined(__irix__) || defined(__sgi)
#define SDLMAME_IRIX 1
/* Large file support on IRIX needs _SGI_SOURCE */
#undef _POSIX_SOURCE

#elif defined(__linux__)
#define SDLMAME_LINUX 1

#elif defined(__FreeBSD__) || defined(__DragonFly__)
#define SDLMAME_FREEBSD 1
#endif

#endif /* SDLMAME_UNIX */

#endif /* _OSDEFS_H_ */
