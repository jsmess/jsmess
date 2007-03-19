/***************************************************************************

    CDRDAO TOC parser for CHD compression frontend

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __CHDCD_H__
#define __CHDCD_H__

#include "cdrom.h"

chd_error cdrom_parse_toc(const char *tocfname, cdrom_toc *outtoc, cdrom_track_input_info *outinfo);

#endif	/* __CHDCD_H__ */
