/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "mscommon.h"

#include "includes/mk1.h"

static unsigned char mk1_palette[] =
{
	0x20, 0x02, 0x05,
	0xc0, 0x00, 0x00
};

static unsigned short mk1_colortable[] =
{
	0, 1
};

PALETTE_INIT( mk1 )
{
	palette_set_colors(machine, 0, mk1_palette, sizeof(mk1_palette) / 3);
	memcpy(colortable, mk1_colortable, sizeof(mk1_colortable));
}

VIDEO_START( mk1 )
{
	/* artwork seams to need this */
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)auto_malloc (videoram_size);
	return 0;
}

UINT8 mk1_led[4]= {0};

static char led[]={
	" ii          hhhhhhhhhhhh\r"
	"iiii    bbb hhhhhhhhhhhhh ggg\r"
	" ii     bbb hhhhhhhhhhhhh ggg\r"
	"        bbb               ggg\r"
	"        bbb      jjj      ggg\r"
	"       bbb       jjj     ggg\r"
	"       bbb       jjj     ggg\r"
	"       bbb      jjj      ggg\r"
	"       bbb      jjj      ggg\r"
	"      bbb       jjj     ggg\r"
	"      bbb       jjj     ggg\r"
	"      bbb      jjj      ggg\r"
	"      bbb      jjj      ggg\r"
	"     bbb       jjj     ggg\r"
	"     bbb       jjj     ggg\r"
	"     bbb               ggg\r"
    "     bbb ccccccccccccc ggg\r"
    "        cccccccccccccc\r"
    "    ddd ccccccccccccc fff\r"
	"    ddd               fff\r"
	"    ddd      kkk      fff\r"
	"    ddd      kkk      fff\r"
	"   ddd       kkk     fff\r"
	"   ddd       kkk     fff\r"
	"   ddd      kkk      fff\r"
	"   ddd      kkk      fff\r"
	"  ddd       kkk     fff\r"
	"  ddd       kkk     fff\r"
	"  ddd      kkk      fff\r"
	"  ddd      kkk      fff\r"
	" ddd       kkk     fff\r"
	" ddd               fff\r"
    " ddd eeeeeeeeeeeee fff   aa\r"
    " ddd eeeeeeeeeeeee fff  aaaa\r"
    "     eeeeeeeeeeee        aa"
};

static void mk1_draw_9segment(mame_bitmap *bitmap,int value, int x, int y)
{
	draw_led(bitmap, led, value, x, y);
}

static struct {
	int x,y;
} mk1_led_pos[4]={
	{102,79},
	{140,79},
	{178,79},
	{216,79}
};

VIDEO_UPDATE( mk1 )
{
	int i;

	for (i = 0; i < 4; i++)
	{
		mk1_draw_9segment(bitmap, mk1_led[i], mk1_led_pos[i].x, mk1_led_pos[i].y);
		mk1_led[i]=0;
	}
	return 0;
}
