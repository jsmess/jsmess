/******************************************************************************

  messsoft.c

  The list of all available software lists. Software lists have to be included
  here to be recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the SOFTWARE_LIST() macro. The first
  one declares external references to the software lists; the second one builds
  an array storing all the software lists.

******************************************************************************/

#include "emu.h"
#include "softlist.h"


#ifndef SOFTWARE_LIST_RECURSIVE

#define SOFTWARE_LIST_RECURSIVE

/* step 1: declare all external references */
#define ADD_SOFTWARE_LIST(NAME) extern const software_list software_list_##NAME;
#include "messsoft.c"

/* step 2: define the software_list[] array */
#undef ADD_SOFTWARE_LIST
#define ADD_SOFTWARE_LIST(NAME) &software_list_##NAME,
const software_list * const software_lists[] =
{
#include "messsoft.c"
  0             /* end of array */
};

#else /* SOFTWARE_LIST_RECURSIVE */

/****************SOFTWARE LISTS**********************************************/

	ADD_SOFTWARE_LIST( _32x_cart )      /* Sega 32X cartridges */
	ADD_SOFTWARE_LIST( advision_cart )  /* Entex Adventure Vision */
	ADD_SOFTWARE_LIST( bbcbc_cart )     /* BBC Bridge Companion cartridges */
	ADD_SOFTWARE_LIST( crvision_cart )  /* V-Tech Creativison cartridges */
	ADD_SOFTWARE_LIST( gamecom_cart )   /* Tiger Game.com cartridges */
	ADD_SOFTWARE_LIST( gamegear_cart )  /* Sega Game Gear cartridges */
	ADD_SOFTWARE_LIST( gamepock_cart )  /* Epoch Game Pocket Computer cartridges */
	ADD_SOFTWARE_LIST( gmaster_cart )   /* Hartung Game Master cartridges */
	ADD_SOFTWARE_LIST( jaguar_cart )    /* Atari Jaguar cartridges */
	ADD_SOFTWARE_LIST( lynx_cart )      /* Atari Lynx cartridges */
	ADD_SOFTWARE_LIST( megadriv_cart )  /* Sega MegaDrive / Genesis cartridges */
	ADD_SOFTWARE_LIST( megasvp_cart )   /* Sega MegaDrive / Genesis w/SVP cartridges */
	ADD_SOFTWARE_LIST( ngp_cart )       /* SNK Neo Geo Pocket cartridges */
	ADD_SOFTWARE_LIST( ngpc_cart )      /* SNK Neo Geo Pocket Color cartridges */
	ADD_SOFTWARE_LIST( pasogo_cart )    /* Koei Pasogo cartridges */
	ADD_SOFTWARE_LIST( pico_cart )      /* Sega Pico cartridges */
	ADD_SOFTWARE_LIST( pokemini_cart )  /* Nintendo Pokémon Mini cartridges */
	ADD_SOFTWARE_LIST( sg1000_cart )    /* Sega SG-1000 & SG-1000 MarkII cartridges */
	ADD_SOFTWARE_LIST( sms_cart )       /* Sega Master System cartridges */
	ADD_SOFTWARE_LIST( spectrum_cart )  /* Sinclair ZX Spectrum cartridges */
	ADD_SOFTWARE_LIST( supracan_cart )  /* Super A'Can cartridges */
	ADD_SOFTWARE_LIST( svision_cart )   /* Watara Supervision cartridges */
	ADD_SOFTWARE_LIST( vboy_cart )      /* Nintendo Virtual Boy cartridges */
	ADD_SOFTWARE_LIST( wswan_cart )     /* Bandai WonderSwan */
	ADD_SOFTWARE_LIST( wscolor_cart )   /* Bandai WonderSwan Color */

#endif /* SOFTWARE_LIST_RECURSIVE */
