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

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/cdislave.h"
#include "includes/cdi.h"

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(machine->device("maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

static void slave_prepare_readback(running_machine *machine, attotime delay, UINT8 channel, UINT8 count, UINT8 data0, UINT8 data1, UINT8 data2, UINT8 data3, UINT8 cmd);
static void perform_mouse_update(running_machine *machine);
static void set_mouse_position(running_machine* machine);

TIMER_CALLBACK( slave_trigger_readback_int )
{
    cdi_state *state = machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;

    verboselog(machine, 0, "Asserting IRQ2\n" );
    cpu_set_input_line_vector(machine->device("maincpu"), M68K_IRQ_2, 26);
    cputag_set_input_line(machine, "maincpu", M68K_IRQ_2, ASSERT_LINE);
    timer_adjust_oneshot(slave->interrupt_timer, attotime_never, 0);
}

static void slave_prepare_readback(running_machine *machine, attotime delay, UINT8 channel, UINT8 count, UINT8 data0, UINT8 data1, UINT8 data2, UINT8 data3, UINT8 cmd)
{
    cdi_state *state = machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;

    slave->channel[channel].out_index = 0;
    slave->channel[channel].out_count = count;
    slave->channel[channel].out_buf[0] = data0;
    slave->channel[channel].out_buf[1] = data1;
    slave->channel[channel].out_buf[2] = data2;
    slave->channel[channel].out_buf[3] = data3;
    slave->channel[channel].out_cmd = cmd;

    timer_adjust_oneshot(slave->interrupt_timer, delay, 0);
}

static void perform_mouse_update(running_machine *machine)
{
    cdi_state *state = machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;
    UINT16 x = input_port_read(machine, "MOUSEX");
    UINT16 y = input_port_read(machine, "MOUSEY");
    UINT8 buttons = input_port_read(machine, "MOUSEBTN");

    UINT16 old_mouse_x = slave->real_mouse_x;
    UINT16 old_mouse_y = slave->real_mouse_y;

    if(slave->real_mouse_x == 0xffff)
    {
        old_mouse_x = x & 0x3ff;
        old_mouse_y = y & 0x3ff;
    }

    slave->real_mouse_x = x & 0x3ff;
    slave->real_mouse_y = y & 0x3ff;

    slave->fake_mouse_x += (slave->real_mouse_x - old_mouse_x);
    slave->fake_mouse_y += (slave->real_mouse_y - old_mouse_y);

    while(slave->fake_mouse_x > 0x3ff)
    {
        slave->fake_mouse_x += 0x400;
    }

    while(slave->fake_mouse_y > 0x3ff)
    {
        slave->fake_mouse_y += 0x400;
    }

    x = slave->fake_mouse_x;
    y = slave->fake_mouse_y;

    if(slave->polling_active)
    {
        slave_prepare_readback(machine, attotime_zero, 0, 4, ((x & 0x380) >> 7) | (buttons << 4), x & 0x7f, (y & 0x380) >> 7, y & 0x7f, 0xf7);
    }
}

INPUT_CHANGED( mouse_update )
{
    perform_mouse_update(field->port->machine);
}

READ16_HANDLER( slave_r )
{
    cdi_state *state = space->machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;

    if(slave->channel[offset].out_count)
    {
        UINT8 ret = slave->channel[offset].out_buf[slave->channel[offset].out_index];
        verboselog(space->machine, 0, "slave_r: Channel %d: %d, %02x\n", offset, slave->channel[offset].out_index, ret );
        if(slave->channel[offset].out_index == 0)
        {
            switch(slave->channel[offset].out_cmd)
            {
                case 0xb0:
                case 0xb1:
                case 0xf0:
                case 0xf3:
                case 0xf7:
                    verboselog(space->machine, 0, "slave_r: De-asserting IRQ2\n" );
                    cputag_set_input_line(space->machine, "maincpu", M68K_IRQ_2, CLEAR_LINE);
                    break;
            }
        }
        slave->channel[offset].out_index++;
        slave->channel[offset].out_count--;
        if(!slave->channel[offset].out_count)
        {
            slave->channel[offset].out_index = 0;
            slave->channel[offset].out_cmd = 0;
            memset(slave->channel[offset].out_buf, 0, 4);
        }
        return ret;
    }
    verboselog(space->machine, 0, "slave_r: Channel %d: %d\n", offset, slave->channel[offset].out_index );
    return 0xff;
}

static void set_mouse_position(running_machine* machine)
{
    cdi_state *state = machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;
    UINT16 x, y;

    //printf( "Set mouse position: %02x %02x %02x\n", slave->in_buf[0], slave->in_buf[1], slave->in_buf[2] );

    slave->fake_mouse_y = ((slave->in_buf[1] & 0x0f) << 6) | (slave->in_buf[0] & 0x3f);
    slave->fake_mouse_x = ((slave->in_buf[1] & 0x70) << 3) | slave->in_buf[2];

    x = slave->fake_mouse_x;
    y = slave->fake_mouse_y;

    if(slave->polling_active)
    {
        //slave_prepare_readback(machine, attotime_zero, 0, 4, (x & 0x380) >> 7, x & 0x7f, (y & 0x380) >> 7, y & 0x7f, 0xf7);
    }
}

WRITE16_HANDLER( slave_w )
{
    cdi_state *state = space->machine->driver_data<cdi_state>();
    slave_regs_t *slave = &state->slave_regs;

    switch(offset)
    {
        case 0:
            if(slave->in_index)
            {
                verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave->in_index, data & 0x00ff );
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                if(slave->in_index == slave->in_count)
                {
                    switch(slave->in_buf[0])
                    {
                        case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
                        case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
                        case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
                        case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
                        case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
                        case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
                        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
                        case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff: // Update Mouse Position
                            set_mouse_position(space->machine);
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            break;
                    }
                }
            }
            else
            {
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                switch(data & 0x00ff)
                {
                    case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
                    case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
                    case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
                    case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
                    case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
                    case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
                    case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
                    case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
                        verboselog(space->machine, 0, "slave_w: Channel %d: Update Mouse Position (0x%02x)\n", offset, data & 0x00ff );
                        slave->in_count = 3;
                        break;
                    default:
                        verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
                        slave->in_index = 0;
                        break;
                }
            }
            break;
        case 1:
            if(slave->in_index)
            {
                verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave->in_index, data & 0x00ff );
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                if(slave->in_index == slave->in_count)
                {
                    switch(slave->in_buf[0])
                    {
                        case 0xf0: // Set Front Panel LCD
                            memcpy(slave->lcd_state, slave->in_buf + 1, 16);
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            break;
                        default:
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            break;
                    }
                }
            }
            else
            {
                switch(data & 0x00ff)
                {
                    default:
                        verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
                        memset(slave->in_buf, 0, 17);
                        slave->in_index = 0;
                        slave->in_count = 0;
                        break;
                }
            }
            break;
        case 2:
            if(slave->in_index)
            {
                verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave->in_index, data & 0x00ff );
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                if(slave->in_index == slave->in_count)
                {
                    switch(slave->in_buf[0])
                    {
                        case 0xf0: // Set Front Panel LCD
                            memset(slave->in_buf + 1, 0, 16);
                            slave->in_count = 17;
                            break;
                        default:
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            break;
                    }
                }
            }
            else
            {
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                switch(data & 0x00ff)
                {
                    case 0x82: // Mute audio
                        verboselog(space->machine, 0, "slave_w: Channel %d: Mute Audio (0x82)\n", offset );
                        dmadac_enable(&state->dmadac[0], 2, 0);
                        slave->in_index = 0;
                        slave->in_count = 0;
                        //timer_adjust_oneshot(cdic->audio_sample_timer, attotime_never, 0);
                        break;
                    case 0x83: // Unmute audio
                        verboselog(space->machine, 0, "slave_w: Channel %d: Unmute Audio (0x83)\n", offset );
                        dmadac_enable(&state->dmadac[0], 2, 1);
                        slave->in_index = 0;
                        slave->in_count = 0;
                        break;
                    case 0xf0: // Set Front Panel LCD
                        verboselog(space->machine, 0, "slave_w: Channel %d: Set Front Panel LCD (0xf0)\n", offset );
                        slave->in_count = 17;
                        break;
                    default:
                        verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
                        memset(slave->in_buf, 0, 17);
                        slave->in_index = 0;
                        slave->in_count = 0;
                        break;
                }
            }
            break;
        case 3:
            if(slave->in_index)
            {
                verboselog(space->machine, 0, "slave_w: Channel %d: %d = %02x\n", offset, slave->in_index, data & 0x00ff );
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                if(slave->in_index == slave->in_count)
                {
                    switch(slave->in_buf[0])
                    {
                        case 0xb0: // Request Disc Status
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(4), 3, 4, 0xb0, 0x00, 0x02, 0x15, 0xb0);
                            break;
                        //case 0xb1: // Request Disc Base
                            //memset(slave->in_buf, 0, 17);
                            //slave->in_index = 0;
                            //slave->in_count = 0;
                            //slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(10000), 3, 4, 0xb1, 0x00, 0x00, 0x00, 0xb1);
                            //break;
                        default:
                            memset(slave->in_buf, 0, 17);
                            slave->in_index = 0;
                            slave->in_count = 0;
                            break;
                    }
                }
            }
            else
            {
                slave->in_buf[slave->in_index] = data & 0x00ff;
                slave->in_index++;
                switch(data & 0x00ff)
                {
                    case 0xb0: // Request Disc Status
                        verboselog(space->machine, 0, "slave_w: Channel %d: Request Disc Status (0xb0)\n", offset );
                        slave->in_count = 4;
                        break;
                    case 0xb1: // Request Disc Status
                        verboselog(space->machine, 0, "slave_w: Channel %d: Request Disc Base (0xb1)\n", offset );
                        slave->in_count = 4;
                        break;
                    case 0xf0: // Get SLAVE revision
                        verboselog(space->machine, 0, "slave_w: Channel %d: Get SLAVE Revision (0xf0)\n", offset );
                        slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(10000), 2, 2, 0xf0, 0x32, 0x31, 0, 0xf0);
                        slave->in_index = 0;
                        break;
                    case 0xf3: // Query Pointer Type
                        verboselog(space->machine, 0, "slave_w: Channel %d: Query Pointer Type (0xf3)\n", offset );
                        slave->in_index = 0;
                        slave_prepare_readback(space->machine, ATTOTIME_IN_HZ(10000), 2, 2, 0xf3, 1, 0, 0, 0xf3);
                        break;
                    case 0xf6: // NTSC/PAL
                        verboselog(space->machine, 0, "slave_w: Channel %d: Check NTSC/PAL (0xf6)\n", offset );
                        slave_prepare_readback(space->machine, attotime_never, 2, 2, 0xf6, 1, 0, 0, 0xf6);
                        slave->in_index = 0;
                        break;
                    case 0xf7: // Activate input polling
                        verboselog(space->machine, 0, "slave_w: Channel %d: Activate Input Polling (0xf7)\n", offset );
                        slave->polling_active = 1;
                        slave->in_index = 0;
                        break;
                    case 0xfa: // Enable X-Bus interrupts
                        verboselog(space->machine, 0, "slave_w: Channel %d: X-Bus Interrupt Enable (0xfa)\n", offset );
                        slave->xbus_interrupt_enable = 1;
                        slave->in_index = 0;
                        break;
                    default:
                        verboselog(space->machine, 0, "slave_w: Channel %d: Unknown register: %02x\n", offset, data & 0x00ff );
                        memset(slave->in_buf, 0, 17);
                        slave->in_index = 0;
                        slave->in_count = 0;
                        break;
                }
            }
            break;
    }
}

void slave_init(running_machine *machine, slave_regs_t *slave)
{
    int index = 0;
    for(index = 0; index < 4; index++)
    {
        slave->channel[index].out_buf[0] = 0;
        slave->channel[index].out_buf[1] = 0;
        slave->channel[index].out_buf[2] = 0;
        slave->channel[index].out_buf[3] = 0;
        slave->channel[index].out_index = 0;
        slave->channel[index].out_count = 0;
        slave->channel[index].out_cmd = 0;
    }

    memset(slave->in_buf, 0, 17);
    slave->in_index = 0;
    slave->in_count = 0;

    slave->polling_active = 0;

    slave->xbus_interrupt_enable = 0;

    memset(slave->lcd_state, 0, 16);

    slave->real_mouse_x = 0;
    slave->real_mouse_y = 0;

    slave->fake_mouse_x = 0;
    slave->fake_mouse_y = 0;
}

void slave_register_globals(running_machine *machine, slave_regs_t *slave)
{
    state_save_register_global(machine, slave->channel[0].out_buf[0]);
    state_save_register_global(machine, slave->channel[0].out_buf[1]);
    state_save_register_global(machine, slave->channel[0].out_buf[2]);
    state_save_register_global(machine, slave->channel[0].out_buf[3]);
    state_save_register_global(machine, slave->channel[0].out_index);
    state_save_register_global(machine, slave->channel[0].out_count);
    state_save_register_global(machine, slave->channel[0].out_cmd);
    state_save_register_global(machine, slave->channel[1].out_buf[0]);
    state_save_register_global(machine, slave->channel[1].out_buf[1]);
    state_save_register_global(machine, slave->channel[1].out_buf[2]);
    state_save_register_global(machine, slave->channel[1].out_buf[3]);
    state_save_register_global(machine, slave->channel[1].out_index);
    state_save_register_global(machine, slave->channel[1].out_count);
    state_save_register_global(machine, slave->channel[1].out_cmd);
    state_save_register_global(machine, slave->channel[2].out_buf[0]);
    state_save_register_global(machine, slave->channel[2].out_buf[1]);
    state_save_register_global(machine, slave->channel[2].out_buf[2]);
    state_save_register_global(machine, slave->channel[2].out_buf[3]);
    state_save_register_global(machine, slave->channel[2].out_index);
    state_save_register_global(machine, slave->channel[2].out_count);
    state_save_register_global(machine, slave->channel[2].out_cmd);
    state_save_register_global(machine, slave->channel[3].out_buf[0]);
    state_save_register_global(machine, slave->channel[3].out_buf[1]);
    state_save_register_global(machine, slave->channel[3].out_buf[2]);
    state_save_register_global(machine, slave->channel[3].out_buf[3]);
    state_save_register_global(machine, slave->channel[3].out_index);
    state_save_register_global(machine, slave->channel[3].out_count);
    state_save_register_global(machine, slave->channel[3].out_cmd);

    state_save_register_global_array(machine, slave->in_buf);
    state_save_register_global(machine, slave->in_index);
    state_save_register_global(machine, slave->in_count);

    state_save_register_global(machine, slave->polling_active);

    state_save_register_global(machine, slave->xbus_interrupt_enable);

    state_save_register_global_array(machine, slave->lcd_state);

    state_save_register_global(machine, slave->real_mouse_x);
    state_save_register_global(machine, slave->real_mouse_y);

    state_save_register_global(machine, slave->fake_mouse_x);
    state_save_register_global(machine, slave->fake_mouse_y);
}
