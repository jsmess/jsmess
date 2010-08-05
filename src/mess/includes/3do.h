/*****************************************************************************
 *
 * includes/3do.h
 *
 ****************************************************************************/

#ifndef _3DO_H_
#define _3DO_H_

class _3do_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, _3do_state(machine)); }

	_3do_state(running_machine &machine)
		: driver_data_t(machine) { }

	legacy_cpu_device* maincpu;
	UINT32 *dram;
	UINT32 *vram;
};

/*----------- defined in machine/3do.c -----------*/

READ32_HANDLER( _3do_nvarea_r );
WRITE32_HANDLER( _3do_nvarea_w );

READ32_HANDLER( _3do_slow2_r );
WRITE32_HANDLER( _3do_slow2_w );
void _3do_slow2_init( running_machine *machine );

READ32_HANDLER( _3do_svf_r );
WRITE32_HANDLER( _3do_svf_w );

READ32_HANDLER( _3do_madam_r );
WRITE32_HANDLER( _3do_madam_w );
void _3do_madam_init( running_machine *machine );

READ32_HANDLER( _3do_clio_r );
WRITE32_HANDLER( _3do_clio_w );
void _3do_clio_init( running_machine *machine, screen_device *screen );


VIDEO_START( _3do );
VIDEO_UPDATE( _3do );

#endif /* _3DO_H_ */
