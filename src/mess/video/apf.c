/******************************************************************************

******************************************************************************/

#include "driver.h"
#include "video/m6847.h"
#include "includes/apf.h"

UINT8 *apf_video_ram;
UINT8 apf_m6847_attr;
static UINT8 apf_chars[0x20], apf_video[0x20];

static ATTR_CONST UINT8 apf_get_attributes(UINT8 c)
{

	/* this seems to be the same so far, as it gives the same result as vapf */
  UINT8 result = apf_m6847_attr;
  if (apf_m6847_attr&M6847_AG) {
    result |= M6847_GM2|M6847_GM1;
  } else {
	if (c & 0x40)	result |= M6847_INV;
	if (c & 0x80)	result |= M6847_AS;
  }
	return result;
}

static UINT8 apf_get_character(int character, int line)
{
  return apf_video_ram[(character&0x1f)*0x10+line+0x200];
}

static void apf_vsync_int(int line)
{
	extern unsigned int apf_ints;
	if (line)
		apf_ints |= 0x10;
	else
		apf_ints &= ~0x10;
	apf_update_ints();
}



static const UINT8 *apf_get_video_ram(int scanline)
{
  if (apf_m6847_attr&M6847_AG) {
    /* apf has only 1kbyte ram and can't use all for video of course
       it doesn't combine the 6847 with the 6883, instead it has own circuit
       to do special 32x12 tilemap at 0x0000 with 8x16 sized characters. The 32 characters are defined at address 0x0200
       and delivers resulting byte to the m6847 which operates in graphics mode
     */
    int i;
    if ((scanline&0xf)==0)
      memcpy (apf_chars, &apf_video_ram[(scanline/16)*0x20], sizeof(apf_chars));
    for (i=0; i<sizeof(apf_video); i++) 
      apf_video[i]=apf_get_character(apf_chars[i], scanline&0xf);
    return apf_video;
  } else {
    return &apf_video_ram[(scanline / 12) * 0x20 + 0x200];
  }
}



VIDEO_START(apf)
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_NTSC;
	cfg.field_sync_callback = apf_vsync_int;
	cfg.get_attributes = apf_get_attributes;
	cfg.get_video_ram = apf_get_video_ram;
	m6847_init(&cfg);
}

VIDEO_UPDATE(apf)
{
    return VIDEO_UPDATE_CALL(m6847);
}
