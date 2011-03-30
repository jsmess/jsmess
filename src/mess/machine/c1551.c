/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - byte latching does not match hardware behavior
      (CPU skips data bytes if implemented per schematics)
    - activity LED

*/

#include "emu.h"
#include "c1551.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6525tpi.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6510T_TAG		"u2"
#define M6523_0_TAG		"u3"
#define M6523_1_TAG		"ci_u2"

#define SYNC \
	!(c1551->mode && ((c1551->data & G64_SYNC_MARK) == G64_SYNC_MARK))

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1551_t c1551_t;
struct _c1551_t
{
	/* TCBM bus */
	int address;							/* device address - 8 */
	UINT8 tcbm_data;						/* data */
	int status;								/* status */
	int dav;								/* data valid */
	int ack;								/* acknowledge */

	/* motors */
	int stp;								/* stepper motor phase */
	int mtr;								/* spindle motor on */

	/* track */
	UINT8 track_buffer[G64_BUFFER_SIZE];	/* track data buffer */
	int track_len;							/* track length */
	int buffer_pos;							/* current byte position within track buffer */
	int bit_pos;							/* current bit position within track buffer byte */
	int bit_count;							/* current data byte bit counter */
	UINT16 data;							/* data shift register */
	UINT8 yb;								/* GCR data byte */

	/* signals */
	int ds;									/* density select */
	int soe;								/* s? output enable */
	int byte;								/* byte ready */
	int mode;								/* mode (0 = write, 1 = read) */

	/* devices */
	device_t *cpu;
	device_t *tpi0;
	device_t *tpi1;
	device_t *image;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1551_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C1551);
	return (c1551_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c1551_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C1551);
	return (c1551_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( irq_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( irq_tick )
{
	c1551_t *c1551 = get_safe_token(timer.owner());

	device_set_input_line(c1551->cpu, M6502_IRQ_LINE, ASSERT_LINE);
	device_set_input_line(c1551->cpu, M6502_IRQ_LINE, CLEAR_LINE);
}

/*-------------------------------------------------
    TIMER_CALLBACK( bit_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( bit_tick )
{
	device_t *device = (device_t *)ptr;
	c1551_t *c1551 = get_safe_token(device);
	int byte = 0;

	/* shift in data from the read head */
	c1551->data <<= 1;
	c1551->data |= BIT(c1551->track_buffer[c1551->buffer_pos], c1551->bit_pos);
	c1551->bit_pos--;
	c1551->bit_count++;

	if (c1551->bit_pos < 0)
	{
		c1551->bit_pos = 7;
		c1551->buffer_pos++;

		if (c1551->buffer_pos >= c1551->track_len)
		{
			/* loop to the start of the track */
			c1551->buffer_pos = 0;
		}
	}

	if (!SYNC)
	{
		/* SYNC detected */
		c1551->bit_count = 0;
	}

	if (c1551->bit_count > 7)
	{
		/* byte ready */
		c1551->bit_count = 0;
		byte = 1;

		c1551->yb = c1551->data & 0xff;

		if (!c1551->yb)
		{
			/* simulate weak bits with randomness */
			c1551->yb = machine.rand() & 0xff;
		}

		c1551->byte = byte;
	}
}

/*-------------------------------------------------
    c1551_port_r - M6510T port read
-------------------------------------------------*/

static void read_current_track(c1551_t *c1551)
{
	c1551->track_len = G64_BUFFER_SIZE;
	c1551->buffer_pos = 0;
	c1551->bit_pos = 7;
	c1551->bit_count = 0;

	/* read track data */
	floppy_drive_read_track_data_info_buffer(c1551->image, 0, c1551->track_buffer, &c1551->track_len);

	/* extract track length */
	c1551->track_len = floppy_drive_get_current_track_size(c1551->image, 0);
}

/*-------------------------------------------------
    on_disk_change - disk change handler
-------------------------------------------------*/

static void on_disk_change(device_image_interface &image)
{
	c1551_t *c1551 = get_safe_token(image.device().owner());

	read_current_track(c1551);
}

/*-------------------------------------------------
    spindle_motor - spindle motor control
-------------------------------------------------*/

static void spindle_motor(c1551_t *c1551, int mtr)
{
	if (c1551->mtr != mtr)
	{
		if (mtr)
		{
			/* read track data */
			read_current_track(c1551);
		}

		floppy_mon_w(c1551->image, !mtr);
		c1551->bit_timer->enable(mtr);

		c1551->mtr = mtr;
	}
}

/*-------------------------------------------------
    step_motor - stepper motor control
-------------------------------------------------*/

static void step_motor(c1551_t *c1551, int mtr, int stp)
{
	if (mtr && (c1551->stp != stp))
	{
		int tracks = 0;

		switch (c1551->stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 3) tracks--; break;
		case 1:	if (stp == 2) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 3) tracks++; else if (stp == 1) tracks--; break;
		case 3: if (stp == 0) tracks++; else if (stp == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			/* step read/write head */
			floppy_drive_seek(c1551->image, tracks);

			/* read new track data */
			read_current_track(c1551);
		}

		c1551->stp = stp;
	}
}

/*-------------------------------------------------
    c1551_port_r - M6510T port read
-------------------------------------------------*/

static UINT8 c1551_port_r( device_t *device, UINT8 direction )
{
	/*

        bit     description

        P0      STP0A
        P1      STP0B
        P2      MTR0
        P3      ACT0
        P4      WPS
        P5      DS0
        P6      DS1
        P7      BYTE LTCHED

    */

	c1551_t *c1551 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* write protect sense */
	data |= !floppy_wpt_r(c1551->image) << 4;

	/* byte latched */
	data |= (c1551->soe & c1551->byte) << 7;

	return data;
}

/*-------------------------------------------------
    c1551_port_w - M6510T port write
-------------------------------------------------*/

static void c1551_port_w( device_t *device, UINT8 direction, UINT8 data )
{
	/*

        bit     description

        P0      STP0A
        P1      STP0B
        P2      MTR0
        P3      ACT0
        P4      WPS
        P5      DS0
        P6      DS1
        P7      BYTE LTCHED

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	/* spindle motor */
	int mtr = BIT(data, 2);
	spindle_motor(c1551, mtr);

	/* stepper motor */
	int stp = data & 0x03;
	step_motor(c1551, mtr, stp);

	/* TODO activity LED */

	/* density select */
	int ds = (data >> 5) & 0x03;

	if (c1551->ds != ds)
	{
		c1551->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C2040_BITRATE[ds]/4));
		c1551->ds = ds;
	}
}

static const m6502_interface m6510t_intf =
{
	NULL,			/* read_indexed_func */
	NULL,			/* write_indexed_func */
	c1551_port_r,	/* port_read_func */
	c1551_port_w	/* port_write_func */
};

/*-------------------------------------------------
    ADDRESS_MAP( c1551_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1551_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_RAM
	AM_RANGE(0x4000, 0x4007) AM_MIRROR(0x3ff8) AM_DEVREADWRITE(M6523_0_TAG, tpi6525_r, tpi6525_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c1551:c1551", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    tpi6525_interface tpi0_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( tpi0_pa_r )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	return c1551->tcbm_data;
}

static WRITE8_DEVICE_HANDLER( tpi0_pa_w )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	c1551->tcbm_data = data;
}

static READ8_DEVICE_HANDLER( tpi0_pb_r )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	c1551->byte = 0;

	return c1551->yb;
}

static WRITE8_DEVICE_HANDLER( tpi0_pb_w )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	c1551->yb = data;
}

static READ8_DEVICE_HANDLER( tpi0_pc_r )
{
	/*

        bit     description

        PC0     TCBM STATUS0
        PC1     TCBM STATUS1
        PC2     TCBM DEV
        PC3     TCBM ACK
        PC4     MODE
        PC5     JP1
        PC6     _SYNC
        PC7     TCBM DAV

    */

	c1551_t *c1551 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* JP1 */
	data |= c1551->address << 5;

	/* SYNC detect line */
	data |= SYNC << 6;

	/* TCBM data valid */
	data |= c1551->dav << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( tpi0_pc_w )
{
	/*

        bit     description

        PC0     TCBM STATUS0
        PC1     TCBM STATUS1
        PC2     TCBM DEV
        PC3     TCBM ACK
        PC4     MODE
        PC5     JP1
        PC6     _SYNC
        PC7     TCBM DAV

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	/* TCBM status */
	c1551->status = data & 0x03;

	/* TODO TCBM device number */

	/* TCBM acknowledge */
	c1551->ack = BIT(data, 3);

	/* read/write mode */
	c1551->mode = BIT(data, 4);
}

static const tpi6525_interface tpi0_intf =
{
	tpi0_pa_r,
	tpi0_pb_r,
	tpi0_pc_r,
	tpi0_pa_w,
	tpi0_pb_w,
	tpi0_pc_w,
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    tpi6525_interface tpi1_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( tpi1_pa_r )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	return c1551->tcbm_data;
}

static WRITE8_DEVICE_HANDLER( tpi1_pa_w )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	c1551->tcbm_data = data;
}

static READ8_DEVICE_HANDLER( tpi1_pb_r )
{
	/*

        bit     description

        PB0     STATUS0
        PB1     STATUS1
        PB2
        PB3
        PB4
        PB5
        PB6
        PB7

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	return c1551->status;
}

static READ8_DEVICE_HANDLER( tpi1_pc_r )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4
        PC5
        PC6     TCBM DAV
        PC7     TCBM ACK

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* TCBM acknowledge */
	data |= c1551->ack << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( tpi1_pc_w )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4
        PC5
        PC6     TCBM DAV
        PC7     TCBM ACK

    */

	c1551_t *c1551 = get_safe_token(device->owner());

	/* TCBM data valid */
	c1551->dav = BIT(data, 6);
}

static const tpi6525_interface tpi1_intf =
{
	tpi1_pa_r,
	tpi1_pb_r,
	tpi1_pc_r,
	tpi1_pa_w,
	NULL,
	tpi1_pc_w,
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1551 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1551 )
	FLOPPY_OPTION( c1551, "g64", "Commodore 1551 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
	FLOPPY_OPTION( c1551, "d64", "Commodore 1551 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1551_floppy_config
-------------------------------------------------*/

static const floppy_config c1551_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c1551),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1551 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1551 )
	MCFG_CPU_ADD(M6510T_TAG, M6510T, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1551_map)
	MCFG_CPU_CONFIG(m6510t_intf)

	MCFG_TPI6525_ADD(M6523_0_TAG, tpi0_intf) // 6523
	MCFG_TPI6525_ADD(M6523_1_TAG, tpi1_intf) // 6523

	MCFG_TIMER_ADD_PERIODIC("irq", irq_tick, attotime::from_hz(120))

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1551_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c1551 )
-------------------------------------------------*/

ROM_START( c1551 ) // schematic 251860
	ROM_REGION( 0x4000, "c1551", ROMREGION_LOADBYNAME )
	ROM_LOAD( "318001-01.u4", 0x0000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1551 )
-------------------------------------------------*/

static DEVICE_START( c1551 )
{
	c1551_t *c1551 = get_safe_token(device);
	const c1551_config *config = get_safe_config(device);

	/* set address */
	assert((config->address > 7) && (config->address < 10));
	c1551->address = config->address - 8;

	/* find our CPU */
	c1551->cpu = device->subdevice(M6510T_TAG);

	/* find devices */
	c1551->tpi0 = device->subdevice(M6523_0_TAG);
	c1551->tpi1 = device->subdevice(M6523_1_TAG);
	c1551->image = device->subdevice(FLOPPY_0);

	/* install image callbacks */
	floppy_install_unload_proc(c1551->image, on_disk_change);
	floppy_install_load_proc(c1551->image, on_disk_change);

	/* allocate data timer */
	c1551->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_tick), (void *)device);

	/* map TPI1 to host CPU memory space */
	address_space *program = device->machine().device(config->cpu_tag)->memory().space(AS_PROGRAM);
	UINT32 start_address = c1551->address ? 0xfec0 : 0xfef0;

	program->install_legacy_readwrite_handler(*c1551->tpi1, start_address, start_address + 7, FUNC(tpi6525_r), FUNC(tpi6525_w));

	/* register for state saving */
	device->save_item(NAME(c1551->tcbm_data));
	device->save_item(NAME(c1551->status));
	device->save_item(NAME(c1551->dav));
	device->save_item(NAME(c1551->ack));
	device->save_item(NAME(c1551->stp));
	device->save_item(NAME(c1551->mtr));
	device->save_item(NAME(c1551->track_len));
	device->save_item(NAME(c1551->buffer_pos));
	device->save_item(NAME(c1551->bit_pos));
	device->save_item(NAME(c1551->bit_count));
	device->save_item(NAME(c1551->data));
	device->save_item(NAME(c1551->yb));
	device->save_item(NAME(c1551->ds));
	device->save_item(NAME(c1551->soe));
	device->save_item(NAME(c1551->byte));
	device->save_item(NAME(c1551->mode));
}

/*-------------------------------------------------
    DEVICE_RESET( c1551 )
-------------------------------------------------*/

static DEVICE_RESET( c1551 )
{
	c1551_t *c1551 = get_safe_token(device);

	c1551->cpu->reset();
	c1551->tpi0->reset();

	c1551->soe = 1;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1551 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1551_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1551_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1551);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1551);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1551);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1551);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1551");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1551");									break;				
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore Plus/4");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(C1551, c1551);
