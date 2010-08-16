/******************************************************************************


    CD-i Mono-I SLAVE MCU simulation
    -------------------

    MESS implementation by Harmony


*******************************************************************************

STATUS:

- Just enough for the Mono-I CD-i board to work somewhat properly.

TODO:

- Decapping and proper emulation.

*******************************************************************************/

#ifndef _MACHINE_CDISLAVE_H_
#define _MACHINE_CDISLAVE_H_

#include "emu.h"

typedef struct
{
    UINT8 out_buf[4];
    UINT8 out_index;
    UINT8 out_count;
    UINT8 out_cmd;
} slave_channel_t;

typedef struct
{
    slave_channel_t channel[4];
    emu_timer *interrupt_timer;

    UINT8 in_buf[17];
    UINT8 in_index;
    UINT8 in_count;

    UINT8 polling_active;

    UINT8 xbus_interrupt_enable;

    UINT8 lcd_state[16];

    UINT16 real_mouse_x;
    UINT16 real_mouse_y;

    UINT16 fake_mouse_x;
    UINT16 fake_mouse_y;
} slave_regs_t;

// Member functions
extern TIMER_CALLBACK( slave_trigger_readback_int );
extern INPUT_CHANGED( mouse_update );
extern READ16_HANDLER( slave_r );
extern WRITE16_HANDLER( slave_w );
extern void slave_init(running_machine *machine, slave_regs_t *slave);
extern void slave_register_globals(running_machine *machine, slave_regs_t *slave);

#endif // _MACHINE_CDISLAVE_H_
