/******************************************************************************
 *  Microtan 65
 *
 *  machine driver
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 *****************************************************************************/

#include "includes/microtan.h"
#include "devices/cassette.h"
#include "image.h"
#include "devices/snapquik.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

static UINT8 microtan_keypad_column;
static UINT8 microtan_keyboard_ascii;

static mame_timer *microtan_timer = NULL;

static int via_0_irq_line = CLEAR_LINE;
static int via_1_irq_line = CLEAR_LINE;
static int kbd_irq_line = CLEAR_LINE;

static UINT8 keyrows[10] = { 0,0,0,0,0,0,0,0,0,0 };
static char keyboard[8][9][8] = {
    { /* normal */
        { 27,'1','2','3','4','5','6','7'},
        {'8','9','0','-','=','`',127,  9},
        {'q','w','e','r','t','y','u','i'},
        {'o','p','[',']', 13,127,  0,  0},
        {'a','s','d','f','g','h','j','k'},
        {'l',';', 39, 92,  0,'z','x','c'},
        {'v','b','n','m',',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Shift */
        { 27,'!','@','#','$','%','^','&'},
        {'*','(',')','_','+','~',127,  9},
        {'Q','W','E','R','T','Y','U','I'},
        {'O','P','{','}', 13,127,  0,  0},
        {'A','S','D','F','G','H','J','K'},
        {'L',':','"','|',  0,'Z','X','C'},
        {'V','B','N','M','<','>','?',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Control */
        { 27,'1','2','3','4','5','6','7'},
        {'8','9','0','-','=','`',127,  9},
        { 17, 23,  5, 18, 20, 25, 21,  9},
        { 15, 16, 27, 29, 13,127,  0,  0},
        {  1, 19,  4,  6,  7,  8, 10, 11},
        { 12,';', 39, 28,  0, 26, 24,  3},
        { 22,  2, 14, 13,',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Shift+Control */
        { 27,'!',  0,'#','$','%', 30,'&'},
        {'*','(',')', 31,'+','~',127,  9},
        { 17, 23,  5, 18, 20, 25, 21,  9},
        { 15, 16, 27, 29, 13,127,  0,  0},
        {  1, 19,  4,  6,  7,  8, 10, 11},
        { 12,':','"', 28,  0, 26, 24,  3},
        { 22,  2, 14, 13,',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* CapsLock */
        { 27,'1','2','3','4','5','6','7'},
        {'8','9','0','-','=','`',127,  9},
        {'Q','W','E','R','T','Y','U','I'},
        {'O','P','[',']', 13,127,  0,  0},
        {'A','S','D','F','G','H','J','K'},
        {'L',';', 39, 92,  0,'Z','X','C'},
        {'V','B','N','M',',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Shift+CapsLock */
        { 27,'!','@','#','$','%','^','&'},
        {'*','(',')','_','+','~',127,  9},
        {'Q','W','E','R','T','Y','U','I'},
        {'O','P','{','}', 13,127,  0,  0},
        {'A','S','D','F','G','H','J','K'},
        {'L',':','"','|',  0,'Z','X','C'},
        {'V','B','N','M','<','>','?',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Control+CapsLock */
        { 27,'1','2','3','4','5','6','7'},
        {'8','9','0','-','=','`',127,  9},
        { 17, 23,  5, 18, 20, 25, 21,  9},
        { 15, 16, 27, 29, 13,127,  0,  0},
        {  1, 19,  4,  6,  7,  8, 10, 11},
        { 12,';', 39, 28,  0, 26, 24,  3},
        { 22,  2, 14, 13,',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
    { /* Shift+Control+CapsLock */
        { 27,'!',  0,'#','$','%', 30,'&'},
        {'*','(',')', 31,'+','~',127,  9},
        { 17, 23,  5, 18, 20, 25, 21,  9},
        { 15, 16, 27, 29, 13,127,  0,  0},
        {  1, 19,  4,  6,  7,  8, 10, 11},
        { 12,':','"', 28,  0, 26, 24,  3},
        { 22,  2, 14, 13,',','.','/',  0},
        { 10,' ','-',',', 13,'.','0','1'},
        {'2','3','4','5','6','7','8','9'},
    },
};

static void microtan_set_irq_line(void)
{
    /* The 6502 IRQ line is active low and probably driven
       by open collector outputs (guess). Since MAME/MESS use
       a non-0 value for ASSERT_LINE we OR the signals here */
    cpunum_set_input_line(0, 0, via_0_irq_line | via_1_irq_line | kbd_irq_line);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/**************************************************************
 * VIA callback functions for VIA #0
 **************************************************************/
static  READ8_HANDLER (via_0_in_a )
{
    int data = readinputport(10);
    LOG(("microtan_via_0_in_a %02X\n", data));
    return data;
}

static  READ8_HANDLER ( via_0_in_b )
{
    int data = 0xff;
    LOG(("microtan_via_0_in_b %02X\n", data));
    return data;
}

static  READ8_HANDLER ( via_0_in_ca1 )
{
    int data = 1;
    LOG(("microtan_via_0_in_ca1 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_0_in_cb1 )
{
    int data = 1;
    LOG(("microtan_via_0_in_cb1 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_0_in_ca2 )
{
    int data = 1;
    LOG(("microtan_via_0_in_ca2 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_0_in_cb2 )
{
    int data = 1;
    LOG(("microtan_via_0_in_cb2 %d\n", data));
    return data;
}

static WRITE8_HANDLER ( via_0_out_a )
{
    LOG(("microtan_via_0_out_a %02X\n", data));
}

static WRITE8_HANDLER (via_0_out_b )
{
    LOG(("microtan_via_0_out_b %02X\n", data));
    /* bit #7 is the cassette output signal */
    cassette_output(cassette_device_image(), data & 0x80 ? +1.0 : -1.0);
}

static WRITE8_HANDLER ( via_0_out_ca2 )
{
    LOG(("microtan_via_0_out_ca2 %d\n", data));
}

static WRITE8_HANDLER (via_0_out_cb2 )
{
    LOG(("microtan_via_0_out_cb2 %d\n", data));
}

static void via_0_irq(int state)
{
    LOG(("microtan_via_0_irq %d\n", state));
    via_0_irq_line = state;
    microtan_set_irq_line();
}

/**************************************************************
 * VIA callback functions for VIA #1
 **************************************************************/
static  READ8_HANDLER ( via_1_in_a )
{
    int data = 0xff;
    LOG(("microtan_via_1_in_a %02X\n", data));
    return data;
}

static  READ8_HANDLER ( via_1_in_b )
{
    int data = 0xff;
    LOG(("microtan_via_1_in_b %02X\n", data));
    return data;
}

static  READ8_HANDLER ( via_1_in_ca1 )
{
    int data = 1;
    LOG(("microtan_via_1_in_ca1 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_1_in_cb1 )
{
    int data = 1;
    LOG(("microtan_via_1_in_cb1 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_1_in_ca2 )
{
    int data = 1;
    LOG(("microtan_via_1_in_ca2 %d\n", data));
    return data;
}

static  READ8_HANDLER ( via_1_in_cb2 )
{
    int data = 1;
    LOG(("microtan_via_1_in_cb2 %d\n", data));
    return data;
}

static WRITE8_HANDLER ( via_1_out_a )
{
    LOG(("microtan_via_1_out_a %02X\n", data));
}

static WRITE8_HANDLER ( via_1_out_b )
{
    LOG(("microtan_via_1_out_b %02X\n", data));
}

static WRITE8_HANDLER (via_1_out_ca2 )
{
    LOG(("microtan_via_1_out_ca2 %d\n", data));
}

static WRITE8_HANDLER ( via_1_out_cb2 )
{
    LOG(("microtan_via_1_out_cb2 %d\n", data));
}

static void via_1_irq(int state)
{
    LOG(("microtan_via_1_irq %d\n", state));
    via_1_irq_line = state;
    microtan_set_irq_line();
}

/**************************************************************
 * VIA read wrappers
 **************************************************************/
#if VERBOSE
static char *via_name[16] = {
    "PB  ","PA  ","DDRB","DDRA",
    "T1CL","T1CH","T1LL","T1LH",
    "T2CL","T2CH","SR  ","ACR ",
    "PCR ","IFR ","IER ","PANH"
};
#endif

 READ8_HANDLER( microtan_via_0_r )
{
    int data = via_0_r(offset);
    LOG(("microtan_via_0_r %s -> %02X\n", via_name[offset], data));
    return data;
}

 READ8_HANDLER( microtan_via_1_r )
{
    int data = via_1_r(offset);
    LOG(("microtan_via_1_r %s -> %02X\n", via_name[offset], data));
    return data;
}

/**************************************************************
 * VIA write wrappers
 **************************************************************/
WRITE8_HANDLER( microtan_via_0_w )
{
    LOG(("via_0_w (%2d) %s <- %02X\n", offset, via_name[offset], data));
    via_0_w(offset,data);
}

WRITE8_HANDLER( microtan_via_1_w )
{
    LOG(("via_1_w (%2d) %s <- %02X\n", offset, via_name[offset], data));
    via_1_w(offset,data);
}

/**************************************************************
 * VIA interface structure
 **************************************************************/
static struct via6522_interface via6522[2] =
{
    {   /* VIA#1 at bfc0-bfcf*/
        via_0_in_a,   via_0_in_b,
        via_0_in_ca1, via_0_in_cb1,
        via_0_in_ca2, via_0_in_cb2,
        via_0_out_a,  via_0_out_b,
		0, 0,
        via_0_out_ca2,via_0_out_cb2,
        via_0_irq,
    },
    {   /* VIA#1 at bfe0-bfef*/
        via_1_in_a,   via_1_in_b,
        via_1_in_ca1, via_1_in_cb1,
        via_1_in_ca2, via_1_in_cb2,
        via_1_out_a,  via_1_out_b,
		0, 0,
        via_1_out_ca2,via_1_out_cb2,
        via_1_irq,
    }
};

static void microtan_read_cassette(int param)
{
	double level = cassette_input(cassette_device_image());

	LOG(("microtan_read_cassette: %+5d\n", level));
	if (level < -0.07)
		via_set_input_cb2(0,0);
	else if (level > +0.07)
		via_set_input_cb2(0,1);
}

 READ8_HANDLER( microtan_sio_r )
{
    int data;
	data = acia_6551_r(offset);
    LOG(("microtan_sio_r: %d -> %02x\n", offset, data));
    return data;
}

WRITE8_HANDLER( microtan_sio_w )
{
    LOG(("microtan_sio_w: %d <- %02x\n", offset, data));
	acia_6551_w(offset,data);
}


 READ8_HANDLER( microtan_sound_r )
{
    int data = 0xff;
    LOG(("microtan_sound_r: -> %02x\n", data));
    return data;
}

WRITE8_HANDLER( microtan_sound_w )
{
    LOG(("microtan_sound_w: <- %02x\n", data));
}


 READ8_HANDLER ( microtan_bffx_r )
{
    int data = 0xff;
    switch( offset & 3 )
    {
    case  0: /* BFF0: read enables chunky graphics */
        microtan_chunky_graphics = 1;
        LOG(("microtan_bff0_r: -> %02x (chunky graphics on)\n", data));
        break;
    case  1: /* BFF1: read undefined (?) */
        LOG(("microtan_bff1_r: -> %02x\n", data));
        break;
    case  2: /* BFF2: read undefined (?) */
        LOG(("microtan_bff2_r: -> %02x\n", data));
        break;
    default: /* BFF3: read keyboard ASCII value */
        data = microtan_keyboard_ascii;
        LOG(("microtan_bff3_r: -> %02x (keyboard ASCII)\n", data));
    }
    return data;
}


/* This callback is called one clock cycle after BFF2 is written (delayed nmi) */
static void microtan_pulse_nmi(int param)
{
    cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

WRITE8_HANDLER ( microtan_bffx_w )
{
    switch( offset & 3 )
    {
    case 0: /* BFF0: write reset keyboard interrupt flag */
        /* This removes bit 7 from the ASCII value of the last key pressed. */
        LOG(("microtan_bff0_w: %d <- %02x (keyboard IRQ clear )\n", offset, data));
        microtan_keyboard_ascii &= ~0x80;
        kbd_irq_line = CLEAR_LINE;
        microtan_set_irq_line();
        break;
    case 1: /* BFF1: write delayed NMI */
        LOG(("microtan_bff1_w: %d <- %02x (delayed NMI)\n", offset, data));
        timer_set(TIME_IN_CYCLES(8,0), 0, microtan_pulse_nmi);
        break;
    case 2: /* BFF2: write keypad column write (what is this meant for?) */
        LOG(("microtan_bff2_w: %d <- %02x (keypad column)\n", offset, data));
        microtan_keypad_column = data;
        break;
    default: /* BFF3: write disable chunky graphics */
        LOG(("microtan_bff3_w: %d <- %02x (chunky graphics off)\n", offset, data));
        microtan_chunky_graphics = 0;
    }
}

static int microtan_varify_snapshot(UINT8 *data, int size)
{
	if (size == 8263)
	{
		LOG(("microtan_snapshot_id: magic size %d found\n", size));
		return IMAGE_VERIFY_PASS;
	}
	else
	{
		if (4 + data[2] + 256 * data[3] + 1 + 16 + 16 + 16 + 1 + 1 + 16 + 16 + 64 + 7 == size)
		{
			LOG(("microtan_snapshot_id: header RAM size + structures matches filesize %d\n", size));
			return IMAGE_VERIFY_PASS;
		}
	}

    return IMAGE_VERIFY_FAIL;
}

static int parse_intel_hex(UINT8 *snapshot_buff, char *src)
{
    char line[128];
    int row = 0, column = 0, last_addr = 0, last_size = 0;

    while (*src)
    {
        if (*src == '\r' || *src == '\n')
        {
            if (column)
            {
                unsigned int size, addr, null, b[32], cs, n;

                line[column] = '\0';
                row++;
                n = sscanf(line, ":%02x%04x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    &size, &addr, &null,
                    &b[ 0], &b[ 1], &b[ 2], &b[ 3], &b[ 4], &b[ 5], &b[ 6], &b[ 7],
                    &b[ 8], &b[ 9], &b[10], &b[11], &b[12], &b[13], &b[14], &b[15],
                    &b[16], &b[17], &b[18], &b[19], &b[20], &b[21], &b[22], &b[23],
                    &b[24], &b[25], &b[26], &b[27], &b[28], &b[29], &b[30], &b[31],
                    &cs);
                if (n == 0)
                {
                    LOG(("parse_intel_hex: malformed line [%s]\n", line));
                }
                else if (n == 1)
                {
                    LOG(("parse_intel_hex: only size found [%s]\n", line));
                }
                else if (n == 2)
                {
                    LOG(("parse_intel_hex: only size and addr found [%s]\n", line));
                }
                else if (n == 3)
                {
                    LOG(("parse_intel_hex: only size, addr and null found [%s]\n", line));
                }
                else
                if (null != 0)
                {
                    LOG(("parse_intel_hex: warning null byte is != 0 [%s]\n", line));
                }
                else
                {
                    int i, sum;

                    n -= 3;

                    sum = size + (addr & 0xff) + ((addr >> 8) & 0xff);
                    if (n != 32 + 1)
                        cs = b[n-1];

                    last_addr = addr;
                    last_size = n-1;
                    LOG(("parse_intel_hex: %04X", addr));
                    for (i = 0; i < n-1; i++)
                    {
                        sum += b[i];
                        snapshot_buff[addr++] = b[i];
                    }
                    LOG(("-%04X checksum %02X+%02X = %02X\n", addr-1, cs, sum & 0xff, (cs + sum) & 0xff));
                }
            }
            column = 0;
        }
        else
        {
            line[column++] = *src;
        }
        src++;
    }
    /* register preset? */
    if (last_size == 7)
    {
        LOG(("parse_intel_hex: registers (?) at %04X\n", last_addr));
        memcpy(&snapshot_buff[8192+64], &snapshot_buff[last_addr], last_size);
    }
    return INIT_PASS;
}

static int parse_zillion_hex(UINT8 *snapshot_buff, char *src)
{
    char line[128];
    int parsing = 0, row = 0, column = 0;

    while (*src)
    {
        if (parsing)
        {
            if (*src == '}')
                parsing = 0;
            else
            {
                if (*src == '\r' || *src == '\n')
                {
                    if (column)
                    {
                        unsigned int addr, b[8], n;

                        line[column] = '\0';
                        row++;
                        n = sscanf(line, "%x %x %x %x %x %x %x %x %x", &addr, &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7]);
                        if (n == 0)
                        {
                            LOG(("parse_zillion_hex: malformed line [%s]\n", line));
                        }
                        else if (n == 1)
                        {
                            LOG(("parse_zillion_hex: only addr found [%s]\n", line));
                        }
                        else
                        {
                            int i;

                            LOG(("parse_zillion_hex: %04X", addr));
                            for (i = 0; i < n-1; i++)
                                snapshot_buff[addr++] = b[i];
                            LOG(("-%04X\n", addr-1));
                        }
                    }
                    column = 0;
                }
                else
                {
                    line[column++] = *src;
                }
            }
        }
        else
        {
            if (*src == '\r' || *src == '\n')
            {
                if (column)
                {
                    int addr, n;

                    row++;
                    line[column] = '\0';
                    n = sscanf(line, "G%x", (unsigned int *) &addr);
                    if (n == 1 && !snapshot_buff[8192+64+1] && !snapshot_buff[8192+64+1])
                    {
                        LOG(("microtan_hexfile_init: go addr %04X\n", addr));
                        snapshot_buff[8192+64+0] = addr & 0xff;
                        snapshot_buff[8192+64+1] = (addr >> 8) & 0xff;
                    }
                }
                column = 0;
            }
            else
            {
                line[column++] = *src;
            }
            if (*src == '{')
            {
                parsing = 1;
                column = 0;
            }
        }
        src++;
    }
    return INIT_PASS;
}

static void store_key(int key)
{
    LOG(("microtan: store key '%c'\n", key));
    microtan_keyboard_ascii = key | 0x80;
    kbd_irq_line = ASSERT_LINE;
    microtan_set_irq_line();
}

INTERRUPT_GEN( microtan_interrupt )
{
    int mod, row, col, chg, new;
    static int lastrow = 0, mask = 0x00, key = 0x00, repeat = 0, repeater = 0;

    if( repeat )
    {
        if( !--repeat )
            repeater = 4;
    }
    else if( repeater )
    {
        repeat = repeater;
    }

    row = 9;
    new = readinputport(row);
    chg = keyrows[--row] ^ new;
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) { new = readinputport(row); chg = keyrows[--row] ^ new; }
    if (!chg) --row;

    if (row >= 0)
    {
        repeater = 0x00;
        mask = 0x00;
        key = 0x00;
        lastrow = row;
        /* CapsLock LED */
        if( row == 3 && chg == 0x80 )
            set_led_status(1, (keyrows[3] & 0x80) ? 0 : 1);

        if (new & chg)  /* key(s) pressed ? */
        {
            mod = 0;

            /* Shift modifier */
            if ( (keyrows[5] & 0x10) || (keyrows[6] & 0x80) )
                mod |= 1;

            /* Control modifier */
            if (keyrows[3] & 0x40)
                mod |= 2;

            /* CapsLock modifier */
            if (keyrows[3] & 0x80)
                mod |= 4;

            /* find new key */
            mask = 0x01;
            for (col = 0; col < 8; col ++)
            {
                if (chg & mask)
                {
                    new &= mask;
                    key = keyboard[mod][row][col];
                    break;
                }
                mask <<= 1;
            }
            if( key )   /* normal key */
            {
                repeater = 30;
                store_key(key);
            }
            else
            if( (row == 0) && (chg == 0x04) ) /* Ctrl-@ (NUL) */
                store_key(0);
            keyrows[row] |= new;
        }
        else
        {
            keyrows[row] = new;
        }
        repeat = repeater;
    }
    else
    if ( key && (keyrows[lastrow] & mask) && repeat == 0 )
    {
        store_key(key);
    }
}

static void microtan_set_cpu_regs(const UINT8 *snapshot_buff, int base)
{
    LOG(("microtan_snapshot_copy: PC:%02X%02X P:%02X A:%02X X:%02X Y:%02X SP:1%02X",
        snapshot_buff[base+1], snapshot_buff[base+0], snapshot_buff[base+2], snapshot_buff[base+3],
        snapshot_buff[base+4], snapshot_buff[base+5], snapshot_buff[base+6]);
    activecpu_set_reg(M6502_PC, snapshot_buff[base+0] + 256 * snapshot_buff[base+1]));
    activecpu_set_reg(M6502_P, snapshot_buff[base+2]);
    activecpu_set_reg(M6502_A, snapshot_buff[base+3]);
    activecpu_set_reg(M6502_X, snapshot_buff[base+4]);
    activecpu_set_reg(M6502_Y, snapshot_buff[base+5]);
    activecpu_set_reg(M6502_S, snapshot_buff[base+6]);
}

static void microtan_snapshot_copy(UINT8 *snapshot_buff, int snapshot_size)
{
    UINT8 *RAM = memory_region(REGION_CPU1);

    /* check for .DMP file format */
    if (snapshot_size == 8263)
    {
        int i, base;
        /********** DMP format
         * Lower 8k of RAM (0000 to 1fff)
         * 64 bytes of chunky graphics bits (first byte bit is for character at 0200, bit 1=0201, etc)
         * 7 bytes of CPU registers (PCL, PCH, PSW, A, IX, IY, SP)
         */
        LOG(("microtan_snapshot_copy: magic size %d found, assuming *.DMP format\n", snapshot_size));

        base = 0;
        /* 8K of RAM from 0000 to 1fff */
        memcpy(RAM, &snapshot_buff[base], 8192);
        base += 8192;
        /* 64 bytes of chunky graphics info */
        for (i = 0; i < 32*16; i++)
        {
            microtan_chunky_buffer[i] = (snapshot_buff[base+i/8] >> (i&7)) & 1;
            dirtybuffer[i] = 1;
        }
        base += 64;
        microtan_set_cpu_regs(snapshot_buff, base);
    }
    else
    {
        int i, ramend, base;
        /********** M65 format ************************************
         *  2 bytes: File version
         *  2 bytes: RAM size
         *  n bytes: RAM (0000 to RAM Size)
         * 16 bytes: 1st 6522 (0xbfc0 to 0xbfcf)
         * 16 bytes: 2ns 6522 (0xbfe0 to 0xbfef)
         * 16 bytes: Microtan IO (0xbff0 to 0xbfff)
         *  1 byte : Invaders sound (0xbc04)
         *  1 byte : Chunky graphics state (0=off, 1=on)
         * 16 bytes: 1st AY8910 registers
         * 16 bytes: 2nd AY8910 registers
         * 64 bytes: Chunky graphics bits (first byte bit 0 is for character at 0200, bit 1=0201, etc)
         *  7 bytes: CPU registers (PCL, PCH, PSW, A, IX, IY, SP)
         */
        ramend = snapshot_buff[2] + 256 * snapshot_buff[3];
        if (2 + 2 + ramend + 1 + 16 + 16 + 16 + 1 + 1 + 16 + 16 + 64 + 7 != snapshot_size)
        {
            LOG(("microtan_snapshot_copy: size %d doesn't match RAM size %d + structure size\n", snapshot_size, ramend+1));
            return;
        }

        LOG(("microtan_snapshot_copy: size %d found, assuming *.M65 format\n", snapshot_size));
        base = 4;
        memcpy(RAM, &snapshot_buff[base], snapshot_buff[2] + 256 * snapshot_buff[3] + 1);
        base += ramend + 1;

        /* first set of VIA6522 registers */
        for (i = 0; i < 16; i++ )
            via_0_w(i, snapshot_buff[base++]);

        /* second set of VIA6522 registers */
        for (i = 0; i < 16; i++ )
            via_1_w(i, snapshot_buff[base++]);

        /* microtan IO bff0-bfff */
        for (i = 0; i < 16; i++ )
        {
            RAM[0xbff0+i] = snapshot_buff[base++];
            if (i < 4)
                microtan_bffx_w(i,RAM[0xbff0+i]);
        }

        microtan_sound_w(0, snapshot_buff[base++]);
        microtan_chunky_graphics = snapshot_buff[base++];

        /* first set of AY8910 registers */
        for (i = 0; i < 16; i++ )
        {
            AY8910_control_port_0_w(0, i);
            AY8910_write_port_0_w(0, snapshot_buff[base++]);
        }

        /* second set of AY8910 registers */
        for (i = 0; i < 16; i++ )
        {
            AY8910_control_port_0_w(0, i);
            AY8910_write_port_0_w(0, snapshot_buff[base++]);
        }

        for (i = 0; i < 32*16; i++)
        {
            microtan_chunky_buffer[i] = (snapshot_buff[base+i/8] >> (i&7)) & 1;
            dirtybuffer[i] = 1;
        }
        base += 64;

        microtan_set_cpu_regs(snapshot_buff, base);
    }
}

SNAPSHOT_LOAD( microtan )
{
	UINT8 *snapshot_buff;

	snapshot_buff = image_ptr(image);
	if (!snapshot_buff)
		return INIT_FAIL;

	if (microtan_varify_snapshot(snapshot_buff, snapshot_size)==IMAGE_VERIFY_FAIL)
		return INIT_FAIL;

	microtan_snapshot_copy(snapshot_buff, snapshot_size);
	return INIT_PASS;
}

QUICKLOAD_LOAD( microtan_hexfile )
{
	int snapshot_size;
	UINT8 *snapshot_buff;
	char *buff;
	int rc;

	snapshot_size = 8263;   /* magic size */
	snapshot_buff = malloc(snapshot_size);
	if (!snapshot_buff)
	{
		LOG(("microtan_hexfile_load: could not allocate %d bytes of buffer\n", snapshot_size));
		return INIT_FAIL;
	}
	memset(snapshot_buff, 0, snapshot_size);

	buff = malloc(quickload_size + 1);
	if (!buff)
	{
		LOG(("microtan_hexfile_load: could not allocate %d bytes of buffer\n", quickload_size));
		return INIT_FAIL;
	}
	image_fread(image, buff, quickload_size);

    buff[quickload_size] = '\0';

    if (buff[0] == ':')
        rc = parse_intel_hex(snapshot_buff, buff);
	else
		rc = parse_zillion_hex(snapshot_buff, buff);
	if (rc == INIT_PASS)
		microtan_snapshot_copy(snapshot_buff, snapshot_size);
	free(snapshot_buff);
	return rc;
}

DRIVER_INIT( microtan )
{
    UINT8 *dst = memory_region(REGION_GFX2);
    int i;

    for (i = 0; i < 256; i++)
    {
        switch (i & 3)
        {
        case 0: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x00; break;
        case 1: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xf0; break;
        case 2: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x0f; break;
        case 3: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xff; break;
        }
        dst += 4;
        switch ((i >> 2) & 3)
        {
        case 0: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x00; break;
        case 1: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xf0; break;
        case 2: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x0f; break;
        case 3: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xff; break;
        }
        dst += 4;
        switch ((i >> 4) & 3)
        {
        case 0: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x00; break;
        case 1: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xf0; break;
        case 2: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x0f; break;
        case 3: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xff; break;
        }
        dst += 4;
        switch ((i >> 6) & 3)
        {
        case 0: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x00; break;
        case 1: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xf0; break;
        case 2: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0x0f; break;
        case 3: dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = 0xff; break;
        }
        dst += 4;
    }

    switch (readinputport(0) & 3)
    {
        case 0:  // 1K only :)
            memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0xbbff, 0, 0, MRA8_NOP);
            memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0xbbff, 0, 0, MWA8_NOP);
            break;
        case 1:  // +7K TANEX
            memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0x1fff, 0, 0, MRA8_RAM);
            memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0x1fff, 0, 0, MWA8_RAM);
            memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0xbbff, 0, 0, MRA8_NOP);
            memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0xbbff, 0, 0, MWA8_NOP);
            break;
        default: // +7K TANEX + 40K TANRAM
            memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0xbbff, 0, 0, MRA8_RAM);
            memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0400, 0xbbff, 0, 0, MWA8_RAM);
            break;
    }

}

MACHINE_RESET( microtan )
{
    int i;

    for (i = 1; i < 10;  i++)
        keyrows[i] = readinputport(1+i);
    set_led_status(1, (keyrows[3] & 0x80) ? 0 : 1);

    via_config(0, &via6522[0]);
    via_config(1, &via6522[1]);

	acia_6551_init();

	microtan_timer = timer_alloc(microtan_read_cassette);
}
