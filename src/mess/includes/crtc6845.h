/***************************************************************************

    Motorola 6845 CRT controller and emulation

    This code emulates the functionality of the 6845 chip, and also
    supports the functionality of chips related to the 6845

    Peter Trauner
    Nathan Woods

***************************************************************************/

#ifndef MSCRTC6845_H_
#define MSCRTC6845_H_

/***************************************************************************

    Constants

***************************************************************************/

/* since this code is used to support emulations of pseudo-M6845 chips, that
   support functionality simlar to 6845's but are not true M6845 chips, we
   need to have a concept of a personality */
#define M6845_PERSONALITY_GENUINE	0
#define M6845_PERSONALITY_PC1512	1

/***************************************************************************

    Type definitions

***************************************************************************/

/*----------- defined in video/crtc6845.c -----------*/

/* opaque structure representing a crtc8645 chip */
struct mscrtc6845;

struct mscrtc6845_cursor
{
	int on;
	int pos;
	int top;
	int bottom;
};

struct mscrtc6845_config
{
	int freq;
	void (*cursor_changed)(struct mscrtc6845_cursor *old);
	int personality;
};

/***************************************************************************

    Externs

***************************************************************************/

/* generic mscrtc6845 instance */
extern struct mscrtc6845 *mscrtc6845;

/***************************************************************************

    Prototypes

***************************************************************************/

struct mscrtc6845 *mscrtc6845_init(running_machine &machine, const struct mscrtc6845_config *config);

void mscrtc6845_set_clock(struct mscrtc6845 *crtc, int freq);

/* to be called before drawing screen */
void mscrtc6845_time(running_machine &machine, struct mscrtc6845 *crtc);

int mscrtc6845_get_char_columns(struct mscrtc6845 *crtc);
int mscrtc6845_get_char_height(struct mscrtc6845 *crtc);
int mscrtc6845_get_char_lines(struct mscrtc6845 *crtc);
int mscrtc6845_get_start(struct mscrtc6845 *crtc);
void mscrtc6845_set_char_columns(struct mscrtc6845 *crtc, UINT8 columns);
void mscrtc6845_set_char_lines(struct mscrtc6845 *crtc, UINT8 lines);

int mscrtc6845_get_personality(struct mscrtc6845 *crtc);

/* cursor off, cursor on, cursor 16 frames on/off, cursor 32 frames on/off
    start line, end line */
void mscrtc6845_get_cursor(struct mscrtc6845 *crtc, struct mscrtc6845_cursor *cursor);

UINT8 mscrtc6845_port_r(struct mscrtc6845 *crtc, int offset);
int mscrtc6845_port_w(struct mscrtc6845 *crtc, int offset, UINT8 data);

/* to be called when writting to port */
WRITE8_HANDLER ( mscrtc6845_0_port_w );

/* to be called when reading from port */
READ8_HANDLER ( mscrtc6845_0_port_r );

/***************************************************************************

    6845 variant macros

    These are used to support emulations of 6845 variants, but these will
    be eventually merged into mscrtc6845.c so these are deprecated

***************************************************************************/

#define CRTC6845_COLUMNS (REG(0)+1)
#define CRTC6845_CHAR_COLUMNS (REG(1))
#define CRTC6845_LINES (REG(4)*CRTC6845_CHAR_HEIGHT+REG(5))
#define CRTC6845_CHAR_LINES REG(6)
#define CRTC6845_CHAR_HEIGHT ((REG(9)&0x1f)+1)
#define CRTC6845_VIDEO_START ((REG(0xc)<<8)|REG(0xd))
#define CRTC6845_INTERLACE_MODE (REG(8)&3)
#define CRTC6845_INTERLACE_SIGNAL 1
#define CRTC6845_INTERLACE 3
#define CRTC6845_CURSOR_MODE (REG(0xa)&0x60)
#define CRTC6845_CURSOR_OFF 0x20
#define CRTC6845_CURSOR_16FRAMES 0x40
#define CRTC6845_CURSOR_32FRAMES 0x60
#define CRTC6845_SKEW	(REG(8)&15)
#define CRTC6845_CURSOR_POS ((REG(0xe)<<8)|REG(0xf))
#define CRTC6845_CURSOR_TOP	(REG(0xa)&0x1f)
#define CRTC6845_CURSOR_BOTTOM REG(0xb)

#endif /* MSCRTC6845_H_ */
