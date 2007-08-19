/*

	Hitachi H8S/2xxx MCU

	(c) 2001-2007 Tim Schuerewegen

	H8S/2241
	H8S/2246
	H8S/2323

*/

#ifndef _H8SDASM_H_
#define _H8SDASM_H_

#include "driver.h"

offs_t h8s2xxx_disassemble( char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);

#endif
