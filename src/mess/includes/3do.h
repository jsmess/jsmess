/*****************************************************************************
 *
 * includes/3do.h
 *
 ****************************************************************************/

#ifndef _3DO_H_
#define _3DO_H_

class _3do_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, _3do_state(machine)); }

	_3do_state(running_machine &machine) { }

	UINT32 *dram;
	UINT32 *vram;
};

/*----------- defined in machine/3do.c -----------*/

READ32_HANDLER( _3do_nvarea_r );
WRITE32_HANDLER( _3do_nvarea_w );

READ32_HANDLER( _3do_unk_318_r );
WRITE32_HANDLER( _3do_unk_318_w );

READ32_HANDLER( _3do_svf_r );
WRITE32_HANDLER( _3do_svf_w );

READ32_HANDLER( _3do_madam_r );
WRITE32_HANDLER( _3do_madam_w );
void _3do_madam_init( void );

READ32_HANDLER( _3do_clio_r );
WRITE32_HANDLER( _3do_clio_w );
void _3do_clio_init( void );


#endif /* _3DO_H_ */
