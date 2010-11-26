/***************************************************************************

  adam.c

  machine file to handle emulation of the ColecoAdam.

***************************************************************************/

#include "emu.h"
#include "video/tms9928a.h"
#include "includes/adam.h"
#include "devices/flopdrv.h"
#include "devices/cartslot.h"



/* TODO: Are these digits correct? */
static const int KbBufferCapacity = 15; /* Capacity of the Keyboard Buffer */
static const int KbRepeatDelay = 20;


#ifdef UNUSED_FUNCTION
int adam_cart_verify(const UINT8 *cartdata, size_t size)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is in Colecovision format */
	if ((cartdata[0] == 0xAA) && (cartdata[1] == 0x55)) /* Production Cartridge */
		retval = IMAGE_VERIFY_PASS;
	if ((cartdata[0] == 0x55) && (cartdata[1] == 0xAA)) /* "Test" Cartridge. Some games use this method to skip ColecoVision title screen and delay */
		retval = IMAGE_VERIFY_PASS;

	return retval;
}
#endif

void adam_clear_keyboard_buffer(running_machine *machine)
{
	adam_state *state = machine->driver_data<adam_state>();
    int i;

    state->KeyboardBuffer[0] = 0; /* Keys on current buffer */
    for(i=1;i<KbBufferCapacity;i++)
    {
        state->KeyboardBuffer[i] = 0; /* Key pressed */
    }
    for(i=0;i<255;i++)
    {
        state->KbRepeatTable[i]=0; /* Clear Key Repeat Table */
    }

}

static unsigned char getKeyFromBuffer(adam_state *state)
{
	int i;
	UINT8 kbcode;

	kbcode=state->KeyboardBuffer[1];
	for(i=1;i<state->KeyboardBuffer[0];i++)
	{
		state->KeyboardBuffer[i]=state->KeyboardBuffer[i+1];
	}
	state->KeyboardBuffer[state->KeyboardBuffer[0]]=0;
	if (state->KeyboardBuffer[0]>0) state->KeyboardBuffer[0]--;
	return kbcode;
}

static void addToKeyboardBuffer(adam_state *state, unsigned char kbcode)
{
    if (state->KbRepeatTable[kbcode]==0)
    {
        if ((state->KeyboardBuffer[0] < KbBufferCapacity))
        {
            state->KeyboardBuffer[0]++;
            state->KeyboardBuffer[state->KeyboardBuffer[0]] = kbcode;
            state->KbRepeatTable[kbcode]=255-KbRepeatDelay;
        }
    }
}

void adam_explore_keyboard(running_machine *machine)
{
	adam_state *state = machine->driver_data<adam_state>();
    int controlKey, shiftKey;
    int i, keyboard[10];

//logerror("Exploring Keyboard.............\n");
    for(i=0;i<=255;i++) {if (state->KbRepeatTable[i]>0) state->KbRepeatTable[i]++;} /* Update repeat table */

    keyboard[0] = input_port_read(machine, "keyboard_1");
    keyboard[1] = input_port_read(machine, "keyboard_2");
    keyboard[2] = input_port_read(machine, "keyboard_3");
    keyboard[3] = input_port_read(machine, "keyboard_4");
    keyboard[4] = input_port_read(machine, "keyboard_5");
    keyboard[5] = input_port_read(machine, "keyboard_6");
    keyboard[6] = input_port_read(machine, "keyboard_7");
    keyboard[7] = input_port_read(machine, "keyboard_8");
    keyboard[8] = input_port_read(machine, "keyboard_9");
    keyboard[9] = input_port_read(machine, "keyboard_10");

/* Reference: Appendix of COLECO ADAM TECHNICAL MANUAL at http://drushel.cwru.edu/atm/atm.html */

    controlKey = !(keyboard[9]&0x01);                                                /* Control */
    shiftKey = (!(keyboard[8]&0x40) || !(keyboard[8]&0x80) || !(keyboard[9]&0x04));  /* Left shiftKey or Right shiftKey or Lock */

    if (controlKey && !shiftKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(state, 0x00); /* controlKey + 2 */
    else {state->KbRepeatTable[0x00]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(state, 0x01); /* controlKey + A */
    else {state->KbRepeatTable[0x01]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(state, 0x02); /* controlKey + B */
    else {state->KbRepeatTable[0x02]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(state, 0x03); /* controlKey + C */
    else {state->KbRepeatTable[0x03]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(state, 0x04); /* controlKey + D */
    else {state->KbRepeatTable[0x04]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(state, 0x05); /* controlKey + E */
    else {state->KbRepeatTable[0x05]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(state, 0x06); /* controlKey + F */
    else {state->KbRepeatTable[0x06]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(state, 0x07); /* controlKey + G */
    else {state->KbRepeatTable[0x07]=0;}

    if ((controlKey && !shiftKey && !(keyboard[1] & 0x01)) || !(keyboard[9] & 0x02)) addToKeyboardBuffer(state, 0x08); /* controlKey + H or BACKSPACE */
    else {state->KbRepeatTable[0x08]=0;}
    if ((controlKey && !shiftKey && !(keyboard[1] & 0x02)) || !(keyboard[8] & 0x20)) addToKeyboardBuffer(state, 0x09); /* controlKey + I or TAB */
    else {state->KbRepeatTable[0x09]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(state, 0x0A); /* controlKey + J */
    else {state->KbRepeatTable[0x0A]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(state, 0x0B); /* controlKey + K */
    else {state->KbRepeatTable[0x0B]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(state, 0x0C); /* controlKey + L */
    else {state->KbRepeatTable[0x0C]=0;}
    if ((controlKey && !shiftKey && !(keyboard[1] & 0x20)) || !(keyboard[6] & 0x80)) addToKeyboardBuffer(state, 0x0D); /* controlKey + M or RETURN */
    else {state->KbRepeatTable[0x0D]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(state, 0x0E); /* controlKey + N */
    else {state->KbRepeatTable[0x0E]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(state, 0x0F); /* controlKey + O */
    else {state->KbRepeatTable[0x0F]=0;}

    if (controlKey && !shiftKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(state, 0x10); /* controlKey + P */
    else {state->KbRepeatTable[0x10]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(state, 0x11); /* controlKey + Q */
    else {state->KbRepeatTable[0x11]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(state, 0x12); /* controlKey + R */
    else {state->KbRepeatTable[0x12]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(state, 0x13); /* controlKey + S */
    else {state->KbRepeatTable[0x13]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(state, 0x14); /* controlKey + T */
    else {state->KbRepeatTable[0x14]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(state, 0x15); /* controlKey + U */
    else {state->KbRepeatTable[0x15]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(state, 0x16); /* controlKey + V */
    else {state->KbRepeatTable[0x16]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(state, 0x17); /* controlKey + W */
    else {state->KbRepeatTable[0x17]=0;}

    if (controlKey && !shiftKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(state, 0x18); /* controlKey + X */
    else {state->KbRepeatTable[0x18]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(state, 0x19); /* controlKey + Y */
    else {state->KbRepeatTable[0x19]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(state, 0x1A); /* controlKey + Z */
    else {state->KbRepeatTable[0x1A]=0;}
    if ((controlKey && !shiftKey && !(keyboard[5] & 0x80)) || !(keyboard[0] & 0x01)) addToKeyboardBuffer(state, 0x1B); /* controlKey + [ or ESCAPE/WP */
    else {state->KbRepeatTable[0x1B]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x10)) addToKeyboardBuffer(state, 0x1C); /* controlKey + \ */
    else {state->KbRepeatTable[0x1C]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(state, 0x1D); /* controlKey + ] */
    else {state->KbRepeatTable[0x1D]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(state, 0x1E); /* controlKey + ^ */
    else {state->KbRepeatTable[0x1E]=0;}
    if (controlKey && !shiftKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(state, 0x1F); /* controlKey + 6 */
    else {state->KbRepeatTable[0x1F]=0;}

    if (!controlKey && !shiftKey && !(keyboard[6] & 0x40)) addToKeyboardBuffer(state, 0x20); /* SPACEBAR */
    else {state->KbRepeatTable[0x20]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x02)) addToKeyboardBuffer(state, 0x21); /* shiftKey + 1 */
    else {state->KbRepeatTable[0x21]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(state, 0x22); /* shiftKey + ' */
    else {state->KbRepeatTable[0x22]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x08)) addToKeyboardBuffer(state, 0x23); /* shiftKey + 3 */
    else {state->KbRepeatTable[0x23]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x10)) addToKeyboardBuffer(state, 0x24); /* shiftKey + 4 */
    else {state->KbRepeatTable[0x24]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x20)) addToKeyboardBuffer(state, 0x25); /* shiftKey + 5 */
    else {state->KbRepeatTable[0x25]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x80)) addToKeyboardBuffer(state, 0x26); /* shiftKey + 7 */
    else {state->KbRepeatTable[0x26]=0;}
    if (!shiftKey && !controlKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(state, 0x27); /* ' */
    else {state->KbRepeatTable[0x27]=0;}

    if (shiftKey && !controlKey && !(keyboard[5] & 0x02)) addToKeyboardBuffer(state, 0x28); /* shiftKey + 9 */
    else {state->KbRepeatTable[0x28]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x01)) addToKeyboardBuffer(state, 0x29); /* shiftKey + 0 */
    else {state->KbRepeatTable[0x29]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x01)) addToKeyboardBuffer(state, 0x2A); /* shiftKey + 8 */
    else {state->KbRepeatTable[0x2A]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x08)) addToKeyboardBuffer(state, 0x2B); /* + */
    else {state->KbRepeatTable[0x2B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x10)) addToKeyboardBuffer(state, 0x2C); /* , */
    else {state->KbRepeatTable[0x2C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x40)) addToKeyboardBuffer(state, 0x2D); /* - */
    else {state->KbRepeatTable[0x2D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x20)) addToKeyboardBuffer(state, 0x2E); /* . */
    else {state->KbRepeatTable[0x2E]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x40)) addToKeyboardBuffer(state, 0x2F); /* / */
    else {state->KbRepeatTable[0x2F]=0;}

    if (!shiftKey && !controlKey && !(keyboard[4] & 0x01)) addToKeyboardBuffer(state, 0x30); /* 0 */
    else {state->KbRepeatTable[0x30]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x02)) addToKeyboardBuffer(state, 0x31); /* 1 */
    else {state->KbRepeatTable[0x31]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(state, 0x32); /* 2 */
    else {state->KbRepeatTable[0x32]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x08)) addToKeyboardBuffer(state, 0x33); /* 3 */
    else {state->KbRepeatTable[0x33]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x10)) addToKeyboardBuffer(state, 0x34); /* 4 */
    else {state->KbRepeatTable[0x34]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x20)) addToKeyboardBuffer(state, 0x35); /* 5 */
    else {state->KbRepeatTable[0x35]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(state, 0x36); /* 6 */
    else {state->KbRepeatTable[0x36]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x80)) addToKeyboardBuffer(state, 0x37); /* 7 */
    else {state->KbRepeatTable[0x37]=0;}

    if (!shiftKey && !controlKey && !(keyboard[5] & 0x01)) addToKeyboardBuffer(state, 0x38); /* 8 */
    else {state->KbRepeatTable[0x38]=0;}
    if (!shiftKey && !controlKey && !(keyboard[5] & 0x02)) addToKeyboardBuffer(state, 0x39); /* 9 */
    else {state->KbRepeatTable[0x39]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x80)) addToKeyboardBuffer(state, 0x3A); /* shiftKey + ; */
    else {state->KbRepeatTable[0x3A]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x80)) addToKeyboardBuffer(state, 0x3B); /* ; */
    else {state->KbRepeatTable[0x3B]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x10)) addToKeyboardBuffer(state, 0x3C); /* shiftKey + , */
    else {state->KbRepeatTable[0x3C]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x08)) addToKeyboardBuffer(state, 0x3D); /* shiftKey + + */
    else {state->KbRepeatTable[0x3D]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x20)) addToKeyboardBuffer(state, 0x3E); /* shiftKey + . */
    else {state->KbRepeatTable[0x3E]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x40)) addToKeyboardBuffer(state, 0x3F); /* shiftKey + / */
    else {state->KbRepeatTable[0x3F]=0;}

    if (shiftKey && !controlKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(state, 0x40); /* shiftKey + 2 */
    else {state->KbRepeatTable[0x40]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(state, 0x41); /* shiftKey + A */
    else {state->KbRepeatTable[0x41]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(state, 0x42); /* shiftKey + B */
    else {state->KbRepeatTable[0x42]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(state, 0x43); /* shiftKey + C */
    else {state->KbRepeatTable[0x43]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(state, 0x44); /* shiftKey + D */
    else {state->KbRepeatTable[0x44]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(state, 0x45); /* shiftKey + E */
    else {state->KbRepeatTable[0x45]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(state, 0x46); /* shiftKey + F */
    else {state->KbRepeatTable[0x46]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(state, 0x47); /* shiftKey + G */
    else {state->KbRepeatTable[0x47]=0;}

    if (shiftKey && !controlKey && !(keyboard[1] & 0x01)) addToKeyboardBuffer(state, 0x48); /* shiftKey + H */
    else {state->KbRepeatTable[0x48]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x02)) addToKeyboardBuffer(state, 0x49); /* shiftKey + I */
    else {state->KbRepeatTable[0x49]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(state, 0x4A); /* shiftKey + J */
    else {state->KbRepeatTable[0x4A]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(state, 0x4B); /* shiftKey + K */
    else {state->KbRepeatTable[0x4B]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(state, 0x4C); /* shiftKey + L */
    else {state->KbRepeatTable[0x4C]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x20)) addToKeyboardBuffer(state, 0x4D); /* shiftKey + M */
    else {state->KbRepeatTable[0x4D]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(state, 0x4E); /* shiftKey + N */
    else {state->KbRepeatTable[0x4E]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(state, 0x4F); /* shiftKey + O */
    else {state->KbRepeatTable[0x4F]=0;}

    if (shiftKey && !controlKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(state, 0x50); /* shiftKey + P */
    else {state->KbRepeatTable[0x50]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(state, 0x51); /* shiftKey + Q */
    else {state->KbRepeatTable[0x51]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(state, 0x52); /* shiftKey + R */
    else {state->KbRepeatTable[0x52]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(state, 0x53); /* shiftKey + S */
    else {state->KbRepeatTable[0x53]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(state, 0x54); /* shiftKey + T */
    else {state->KbRepeatTable[0x54]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(state, 0x55); /* shiftKey + U */
    else {state->KbRepeatTable[0x55]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(state, 0x56); /* shiftKey + V */
    else {state->KbRepeatTable[0x56]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(state, 0x57); /* shiftKey + W */
    else {state->KbRepeatTable[0x57]=0;}

    if (shiftKey && !controlKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(state, 0x58); /* shiftKey + X */
    else {state->KbRepeatTable[0x58]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(state, 0x59); /* shiftKey + Y */
    else {state->KbRepeatTable[0x59]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(state, 0x5A); /* shiftKey + Z */
    else {state->KbRepeatTable[0x5A]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x80)) addToKeyboardBuffer(state, 0x5B); /* [ */
    else {state->KbRepeatTable[0x5B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x10)) addToKeyboardBuffer(state, 0x5C); /* \ */
    else {state->KbRepeatTable[0x5C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(state, 0x5D); /* ] */
    else {state->KbRepeatTable[0x5D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(state, 0x5E); /* ^ */
    else {state->KbRepeatTable[0x5E]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(state, 0x5F); /* shiftKey + 6 */
    else {state->KbRepeatTable[0x5F]=0;}

    if (shiftKey && !controlKey && !(keyboard[3] & 0x40)) addToKeyboardBuffer(state, 0x60); /* shiftKey + - */
    else {state->KbRepeatTable[0x60]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(state, 0x61); /* A */
    else {state->KbRepeatTable[0x61]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(state, 0x62); /* B */
    else {state->KbRepeatTable[0x62]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(state, 0x63); /* C */
    else {state->KbRepeatTable[0x63]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(state, 0x64); /* D */
    else {state->KbRepeatTable[0x64]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(state, 0x65); /* E */
    else {state->KbRepeatTable[0x65]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(state, 0x66); /* F */
    else {state->KbRepeatTable[0x66]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(state, 0x67); /* G */
    else {state->KbRepeatTable[0x67]=0;}

    if (!controlKey && !shiftKey && !(keyboard[1] & 0x01)) addToKeyboardBuffer(state, 0x68); /* H */
    else {state->KbRepeatTable[0x68]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x02)) addToKeyboardBuffer(state, 0x69); /* I */
    else {state->KbRepeatTable[0x69]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(state, 0x6A); /* J */
    else {state->KbRepeatTable[0x6A]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(state, 0x6B); /* K */
    else {state->KbRepeatTable[0x6B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(state, 0x6C); /* L */
    else {state->KbRepeatTable[0x6C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x20)) addToKeyboardBuffer(state, 0x6D); /* M */
    else {state->KbRepeatTable[0x6D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(state, 0x6E); /* N */
    else {state->KbRepeatTable[0x6E]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(state, 0x6F); /* O */
    else {state->KbRepeatTable[0x6F]=0;}

    if (!controlKey && !shiftKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(state, 0x70); /* P */
    else {state->KbRepeatTable[0x70]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(state, 0x71); /* Q */
    else {state->KbRepeatTable[0x71]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(state, 0x72); /* R */
    else {state->KbRepeatTable[0x72]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(state, 0x73); /* S */
    else {state->KbRepeatTable[0x73]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(state, 0x74); /* T */
    else {state->KbRepeatTable[0x74]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(state, 0x75); /* U */
    else {state->KbRepeatTable[0x75]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(state, 0x76); /* V */
    else {state->KbRepeatTable[0x76]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(state, 0x77); /* W */
    else {state->KbRepeatTable[0x77]=0;}

    if (!controlKey && !shiftKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(state, 0x78); /* X */
    else {state->KbRepeatTable[0x78]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(state, 0x79); /* Y */
    else {state->KbRepeatTable[0x79]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(state, 0x7A); /* Z */
    else {state->KbRepeatTable[0x7A]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x80)) addToKeyboardBuffer(state, 0x7B); /* shiftKey + [ */
    else {state->KbRepeatTable[0x7B]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x10)) addToKeyboardBuffer(state, 0x7C); /* shiftKey + \ */
    else {state->KbRepeatTable[0x7C]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(state, 0x7D); /* shiftKey + ] */
    else {state->KbRepeatTable[0x7D]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(state, 0x7E); /* shiftKey + ^ */
    else {state->KbRepeatTable[0x7E]=0;}
    if (controlKey && !shiftKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(state, 0x7F); /* controlKey + DELETE */
    else {state->KbRepeatTable[0x7F]=0;}

    if (!shiftKey && !(keyboard[8] & 0x01)) addToKeyboardBuffer(state, 0x80); /* HOME */
    else {state->KbRepeatTable[0x80]=0;}
    if (!shiftKey && !(keyboard[6] & 0x01)) addToKeyboardBuffer(state, 0x81); /* I */
    else {state->KbRepeatTable[0x81]=0;}
    if (!shiftKey && !(keyboard[6] & 0x02)) addToKeyboardBuffer(state, 0x82); /* II */
    else {state->KbRepeatTable[0x82]=0;}
    if (!shiftKey && !(keyboard[6] & 0x04)) addToKeyboardBuffer(state, 0x83); /* III */
    else {state->KbRepeatTable[0x83]=0;}
    if (!shiftKey && !(keyboard[6] & 0x08)) addToKeyboardBuffer(state, 0x84); /* IV */
    else {state->KbRepeatTable[0x84]=0;}
    if (!shiftKey && !(keyboard[6] & 0x10)) addToKeyboardBuffer(state, 0x85); /* V */
    else {state->KbRepeatTable[0x85]=0;}
    if (!shiftKey && !(keyboard[6] & 0x20)) addToKeyboardBuffer(state, 0x86); /* VI */
    else {state->KbRepeatTable[0x86]=0;}
    /* 0x87 - Unused */

    /* 0x88 - Unused */
    if (shiftKey && !controlKey && !(keyboard[6] & 0x01)) addToKeyboardBuffer(state, 0x89); /* shiftKey + I */
    else {state->KbRepeatTable[0x89]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x02)) addToKeyboardBuffer(state, 0x8A); /* shiftKey + II */
    else {state->KbRepeatTable[0x8A]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x04)) addToKeyboardBuffer(state, 0x8B); /* shiftKey + III */
    else {state->KbRepeatTable[0x8B]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x08)) addToKeyboardBuffer(state, 0x8C); /* shiftKey + IV */
    else {state->KbRepeatTable[0x8C]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x10)) addToKeyboardBuffer(state, 0x8D); /* shiftKey + V */
    else {state->KbRepeatTable[0x8D]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x20)) addToKeyboardBuffer(state, 0x8E); /* shiftKey + VI */
    else {state->KbRepeatTable[0x8E]=0;}
    /* 0x8F - Unused */

    if (!shiftKey && !(keyboard[7] & 0x01)) addToKeyboardBuffer(state, 0x90); /* WILD CARD */
    else {state->KbRepeatTable[0x90]=0;}
    if (!shiftKey && !(keyboard[7] & 0x02)) addToKeyboardBuffer(state, 0x91); /* UNDO */
    else {state->KbRepeatTable[0x91]=0;}
    if (!shiftKey && !(keyboard[7] & 0x04)) addToKeyboardBuffer(state, 0x92); /* MOVE */
    else {state->KbRepeatTable[0x92]=0;}
    if (!shiftKey && !(keyboard[7] & 0x08)) addToKeyboardBuffer(state, 0x93); /* STORE */
    else {state->KbRepeatTable[0x93]=0;}
    if (!shiftKey && !(keyboard[7] & 0x10)) addToKeyboardBuffer(state, 0x94); /* INSERT */
    else {state->KbRepeatTable[0x94]=0;}
    if (!shiftKey && !(keyboard[7] & 0x20)) addToKeyboardBuffer(state, 0x95); /* PRINT */
    else {state->KbRepeatTable[0x95]=0;}
    if (!shiftKey && !(keyboard[7] & 0x40)) addToKeyboardBuffer(state, 0x96); /* CLEAR */
    else {state->KbRepeatTable[0x96]=0;}
    if (!shiftKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(state, 0x97); /* DELETE */
    else {state->KbRepeatTable[0x97]=0;}

    if (shiftKey && !controlKey && !(keyboard[7] & 0x01)) addToKeyboardBuffer(state, 0x98); /* shiftKey + WILD CARD */
    else {state->KbRepeatTable[0x98]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x02)) addToKeyboardBuffer(state, 0x99); /* shiftKey + UNDO */
    else {state->KbRepeatTable[0x99]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x04)) addToKeyboardBuffer(state, 0x9A); /* shiftKey + MOVE */
    else {state->KbRepeatTable[0x9A]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x08)) addToKeyboardBuffer(state, 0x9B); /* shiftKey + STORE */
    else {state->KbRepeatTable[0x9B]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x10)) addToKeyboardBuffer(state, 0x9C); /* shiftKey + INSERT */
    else {state->KbRepeatTable[0x9C]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x20)) addToKeyboardBuffer(state, 0x9D); /* shiftKey + PRINT */
    else {state->KbRepeatTable[0x9D]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x40)) addToKeyboardBuffer(state, 0x9E); /* shiftKey + CLEAR */
    else {state->KbRepeatTable[0x9E]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(state, 0x9F); /* shiftKey + DELETE */
    else {state->KbRepeatTable[0x9F]=0;}

    if (!controlKey && !(keyboard[8] & 0x02)) addToKeyboardBuffer(state, 0xA0); /* UP ARROW */
    else {state->KbRepeatTable[0xA0]=0;}
    if (!controlKey && !(keyboard[8] & 0x10)) addToKeyboardBuffer(state, 0xA1); /* RIGHT ARROW */
    else {state->KbRepeatTable[0xA1]=0;}
    if (!controlKey && !(keyboard[8] & 0x04)) addToKeyboardBuffer(state, 0xA2); /* DOWN ARROW */
    else {state->KbRepeatTable[0xA2]=0;}
    if (!controlKey && !(keyboard[8] & 0x08)) addToKeyboardBuffer(state, 0xA3); /* LEFT ARROW */
    else {state->KbRepeatTable[0xA3]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x02)) addToKeyboardBuffer(state, 0xA4); /* controlKey + UP ARROW */
    else {state->KbRepeatTable[0xA4]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x10)) addToKeyboardBuffer(state, 0xA5); /* controlKey + RIGHT ARROW */
    else {state->KbRepeatTable[0xA5]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x04)) addToKeyboardBuffer(state, 0xA6); /* controlKey + DOWN ARROW */
    else {state->KbRepeatTable[0xA6]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x08)) addToKeyboardBuffer(state, 0xA7); /* controlKey + LEFT ARROW */
    else {state->KbRepeatTable[0xA7]=0;}

    if (!controlKey && !(keyboard[8] & 0x02) && !(keyboard[8] & 0x10)) addToKeyboardBuffer(state, 0xA8); /* UP ARROW + RIGHT ARROW */
    else {state->KbRepeatTable[0xA8]=0;}
    if (!controlKey && !(keyboard[8] & 0x10) && !(keyboard[8] & 0x04)) addToKeyboardBuffer(state, 0xA9); /* RIGHT ARROW + DOWN ARROW */
    else {state->KbRepeatTable[0xA9]=0;}
    if (!controlKey && !(keyboard[8] & 0x04) && !(keyboard[8] & 0x08)) addToKeyboardBuffer(state, 0xAA); /* DOWN ARROW + LEFT ARROW */
    else {state->KbRepeatTable[0xAA]=0;}
    if (!controlKey && !(keyboard[8] & 0x08) && !(keyboard[8] & 0x02)) addToKeyboardBuffer(state, 0xAB); /* LEFT ARROW + UP ARROW */
    else {state->KbRepeatTable[0xAB]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x02)) addToKeyboardBuffer(state, 0xAC); /* HOME + UP ARROW */
    else {state->KbRepeatTable[0xAC]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x10)) addToKeyboardBuffer(state, 0xAD); /* HOME + RIGHT ARROW */
    else {state->KbRepeatTable[0xAD]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x04)) addToKeyboardBuffer(state, 0xAE); /* HOME + DOWN ARROW */
    else {state->KbRepeatTable[0xAE]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x08)) addToKeyboardBuffer(state, 0xAF); /* HOME + LEFT ARROW */
    else {state->KbRepeatTable[0xAF]=0;}

    /* 0xB0-0xB7 - Unused */

    if (shiftKey && !controlKey && !(keyboard[9] & 0x02)) addToKeyboardBuffer(state, 0xB8); /* shiftKey + BACKSPACE */
    else {state->KbRepeatTable[0xB8]=0;}
    if (shiftKey && !controlKey && !(keyboard[8] & 0x20)) addToKeyboardBuffer(state, 0xB9); /* shiftKey + TAB */
    else {state->KbRepeatTable[0xB9]=0;}
    /* 0xBA-0xEF - Unused */

    /* 0xF0-0xFF - Reserved for internal use by keyboard software */
}

static void master6801_behaviour(running_machine *machine, int offset, int data)
{
	adam_state *state = machine->driver_data<adam_state>();
/*
The Master MC6801 controls all AdamNet operations, keyboard, tapes, disks...
We are going to "simulate" the behaviour of the master 6801, because we does not have the 2kbytes ROM DUMP
If you have the source listing or the Rom dump, please send us.
*/
	int DCB_Num, deviceNum, statusDCB;
	int i, buffer, /*byteCount,*/ sectorNmbr, /*sectorCount,*/ currentSector;
	UINT8 kbcode;
	static const UINT8 interleave[8] = {0,5,2,7,4,1,6,3};
	device_image_interface *image;
	UINT8 *ram;

	ram = memory_region(machine, "maincpu");
	if (offset == state->pcb)
	{
		switch (data)
		{
			case 1: /* Request to synchronize the Z80 clock */
				ram[offset] = 0x81;
				//logerror("Synchronizing the Z80 clock\n");
				break;
			case 2: /* Request to synchronize the Master 6801 clock */
				ram[offset] = 0x82;
				//logerror("Synchronizing the Master 6801 clock\n");
				break;
			case 3: /* Request to relocate pcb */
				ram[offset] = 0x83; /* Must return 0x83 if success */
				//ram[offset] = 0x9B; /* Time Out */
				logerror("Request to relocate pcb from %04Xh to %04Xh... not implemented... but returns OK\n", state->pcb, (ram[state->pcb+1]|ram[state->pcb+2]<<8));
				break;
		}
	}

	for(DCB_Num=1;DCB_Num<=ram[state->pcb+3];DCB_Num++) /* Test status of each DCB in pcb table */
	{
		statusDCB = (state->pcb+4)+(DCB_Num-1)*21;
		if (offset==statusDCB)
		{
			//logerror("Accesing DCB %02Xh\n", DCB_Num);
			deviceNum = (ram[statusDCB+0x10]&0x0F)+(ram[statusDCB+0x09]<<4);
			buffer=(ram[statusDCB+0x01])+(ram[statusDCB+0x02]<<8);
			//byteCount=(ram[statusDCB+0x03])+(ram[statusDCB+0x04]<<8);

			if (deviceNum>=4 && deviceNum<=7)
			{
				image = dynamic_cast<device_image_interface *>(floppy_get_device(machine, deviceNum - 4));
				if (image->exists())
					ram[statusDCB+20] = (ram[statusDCB+20]&0xF0); /* Inserted Media */
				else
					ram[statusDCB+20] = (ram[statusDCB+20]&0xF0)|0x03; /* No Media on Drive*/
			}
			switch (data)
			{
				case 0: /* Initialize Drive */
					if (deviceNum>=4 && deviceNum<=7)
					{
						ram[statusDCB] = 0x80;
					}
					break;
				case 1: /* Return current status */
					if (deviceNum==1||deviceNum==2)
					{
						ram[statusDCB] = 0x80; /* Device Found (1=Keyboard, 2=Printer) */
						ram[statusDCB+0x13] = 0x01; /* Character device */
					}
					else if (deviceNum>=4 && deviceNum<=7)
					{
						image = dynamic_cast<device_image_interface *>(floppy_get_device(machine, deviceNum - 4));
						if (image->exists())
						{
							ram[statusDCB] = 0x80;
							ram[statusDCB+17] = 1024&255;
							ram[statusDCB+18] = 1024>>8;
						}
						else
						{
							ram[statusDCB] = 0x83; /* Device Found but No Disk in Drive*/
						}
					}
					else
					{
						ram[statusDCB] = 0x9B; /* Time Out - No Device Found*/
					}
					//logerror("   Requesting Status Device %02d=%02Xh\n", deviceNum, ram[statusDCB]);
					break;
				case 2: /* Soft reset */
					ram[statusDCB] = 0x80;
					//logerror("   Reseting Device %02d\n", deviceNum);
					break;
				case 3: /* Write Data */
					//logerror("   Requesting Write to Device %02d\n", deviceNum);
					if (deviceNum==2)
					{
						ram[statusDCB] = 0x80; /* Ok, char sent to printer... no really */
						//logerror("   Requesting Write %2d bytes on buffer [%04xh] to Device %02d\n", byteCount, buffer,deviceNum);
					}
					else
					{
						ram[statusDCB] = 0x85; /* Write Protected Media */
					}
					break;
				case 4: /* Read Data */
					/*
                    AdamNet Error codes on low nibble for tape1,disk1,disk2 and on high nibble for tape2 (share DCB with tape1):

                        0x00  No Error (everything is OK)
                        0x01  CRC Error (block corrupt; data failed cyclic redundancy check)
                        0x02  Missing Block (attempt to access past physical end of medium)
                        0x03  Missing Media (not in drive or drive door open)
                        0x04  Missing Drive (not connected or not turned on)
                        0x05  Write-Protected (write-protect tab covered)
                        0x06  Drive Error (controller or seek failure)
                    */
					if (deviceNum==1)
					{
						kbcode=getKeyFromBuffer(state);
						if (kbcode>0)
						{
							ram[buffer] = kbcode; /* Key pressed  */
							ram[statusDCB] = 0x80;
						}
						else
						{
							ram[statusDCB] = 0x8C; /* No key pressed */
						}
					}
					else if (deviceNum>=4 && deviceNum<=7)
					{
						image = dynamic_cast<device_image_interface *>(floppy_get_device(machine, deviceNum - 4));
						if (image->exists())
						{
							sectorNmbr = ((ram[statusDCB+5])+(ram[statusDCB+6]<<8)+(ram[statusDCB+7]<<16)+(ram[statusDCB+8]<<24))<<1;
							/* sectorCount = (byteCount/512)+(byteCount%512==0)? 0:1; */
							for(i=0;i<=1;i++)
							{
								currentSector = floppy_drive_get_current_track(&image->device());
								while (currentSector > ((sectorNmbr+i)/8))
								{
									floppy_drive_seek(&image->device(), -1);
									currentSector--;
								}
								while (currentSector < ((sectorNmbr+i)/8))
								{
									floppy_drive_seek(&image->device(), 1);
									currentSector++;
								}
								floppy_drive_read_sector_data(&image->device(), 0, interleave[(sectorNmbr+i)&0x07], &ram[buffer+(512*i)],512);
							}
							ram[statusDCB+20] |= 6;
							ram[statusDCB] = 0x80;
						}

					}
					else
					{
						ram[statusDCB] = 0x9B;
					}
					//logerror("   Requesting Read %2d bytes on buffer [%04xh] from Device %02d\n", byteCount, buffer,deviceNum);
					break;
				default:
					ram[statusDCB] = 0x9B; /* Other */
					break;
			}
		}
	}
}

WRITE8_HANDLER( adam_common_writes_w )
{
	adam_state *state = space->machine->driver_data<adam_state>();
    switch (state->upper_memory)
    {
        case 0: /* Internal RAM */
            memory_region(space->machine, "maincpu")[0x08000+offset] = data;
            if (offset>=(state->pcb-0x08000)) master6801_behaviour(space->machine, offset+0x08000, data);
            break;
        case 1: /* ROM Expansion */
            break;
        case 2: /* RAM Expansion */
            memory_region(space->machine, "maincpu")[0x18000+offset] = data;
        	break;
    	case 3: /* Cartridge ROM */
        	break;
    }
}

 READ8_HANDLER( adamnet_r )
{
	adam_state *state = space->machine->driver_data<adam_state>();
    //logerror("adam_net_data Read %2xh\n",state->net_data);
    return state->net_data;
}

WRITE8_HANDLER( adamnet_w )
{
	adam_state *state = space->machine->driver_data<adam_state>();
	/*
    If SmartWriter ROM is selected on lower Z80 memory
    if data bit1 is 1 -> Lower 32k = EOS otherwise Lower 32k = SmartWriter
    */
	UINT8 *BankBase;
	BankBase = &memory_region(space->machine, "maincpu")[0x00000];

	if (data==0x0F)
		adam_reset_pcb(space->machine);
	if ( (state->lower_memory==0) && ((data&0x02)!=(state->net_data&0x02)) )
	{
		if (data&0x02)
		{
			memory_set_bankptr(space->machine, "bank1", BankBase+0x32000); /* No data here */
			memory_set_bankptr(space->machine, "bank2", BankBase+0x34000); /* No data here */
			memory_set_bankptr(space->machine, "bank3", BankBase+0x36000); /* No data here */
			memory_set_bankptr(space->machine, "bank4", BankBase+0x38000); /* EOS ROM */

			memory_set_bankptr(space->machine, "bank6", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank7", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank8", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank9", BankBase+0x3A000); /* Write protecting ROM */
		}
		else
		{
			memory_set_bankptr(space->machine, "bank1", BankBase+0x20000); /* SmartWriter ROM */
			memory_set_bankptr(space->machine, "bank2", BankBase+0x22000);
			memory_set_bankptr(space->machine, "bank3", BankBase+0x24000);
			memory_set_bankptr(space->machine, "bank4", BankBase+0x26000);

			memory_set_bankptr(space->machine, "bank6", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank7", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank8", BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(space->machine, "bank9", BankBase+0x3A000); /* Write protecting ROM */
		}
	}
	state->net_data = data;
}

READ8_HANDLER( adam_memory_map_controller_r )
{
	adam_state *state = space->machine->driver_data<adam_state>();
    return (state->upper_memory << 2) + state->lower_memory;
}

WRITE8_HANDLER( adam_memory_map_controller_w )
{
	adam_state *state = space->machine->driver_data<adam_state>();
    /*
    Writes to ports 0x60 to 0x7F set memory map:

    - D1,D0 -> Lower Memory configuration (0x0000, 0x7FFF)
       0  0    SmartWriter Rom
       0  1    Internal RAM (32k)
       1  0    RAM Expansion (32k)
       1  1    OS7 ROM (8k) + Internal RAM (24k) (for partial ColecoVision compatibility)

    - D3,D2 -> Upper Memory configuration (0x8000, 0xFFFF)
       0  0    Internal RAM (32k)
       0  1    ROM Expansion (32k). The first expansion ROM byte must be 0x66, and the second 0x99.
       1  0    RAM Expansion (32k)
       1  1    Cartridge ROM (32k)

    - D7,D4 -> Reserved for future expansions, must be 0.

    */

    state->lower_memory = (data & 0x03);
    state->upper_memory = (data & 0x0C)>>2;
    adam_set_memory_banks(space->machine);
    //logerror("Configurando la memoria, L:%02xh, U:%02xh\n", state->lower_memory, state->upper_memory);
}

READ8_HANDLER(adam_video_r)
{
    return ((offset&0x01) ? TMS9928A_register_r(space, 1) : TMS9928A_vram_r(space, 0));
}

WRITE8_HANDLER(adam_video_w)
{
    (offset&0x01) ? TMS9928A_register_w(space, 1, data) : TMS9928A_vram_w(space, 0, data);
}

#ifdef UNUSED_FUNCTION
READ8_HANDLER( master6801_ram_r )
{
    /*logerror("Offset %04Xh = %02Xh\n",offset ,memory_region(machine, "maincpu")[offset]);*/
    return memory_region(space->machine, "maincpu")[offset+0x4000];
}

WRITE8_HANDLER( master6801_ram_w )
{
    memory_region(space->machine, "maincpu")[offset+0x4000] = data;
}
#endif

READ8_HANDLER ( adam_paddle_r )
{
	adam_state *state = space->machine->driver_data<adam_state>();

    if (!(input_port_read(space->machine, "controllers") & 0x0F)) return (0xFF); /* If no controllers enabled */
	/* Player 1 */
	if ((offset & 0x02)==0)
	{
		/* Keypad and fire 1 (SAC Yellow Button) */
		if (state->joy_mode==0)
		{
			int inport0,inport1,inport6,data;

			inport0 = input_port_read(space->machine, "controller1_keypad1");
			inport1 = input_port_read(space->machine, "controller1_keypad2");
			inport6 = input_port_read(space->machine, "controllers");

			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
            than one, a real ColecoVision think that it is a third button, so we are going to emulate
            the right behaviour */

			data = 0x0F;	/* No key pressed by default */
			if (!(inport6&0x01)) /* If Driving Controller enabled -> no keypad 1 */
			{
			    if (!(inport0 & 0x01)) data &= 0x0A; /* 0 */
			    if (!(inport0 & 0x02)) data &= 0x0D; /* 1 */
			    if (!(inport0 & 0x04)) data &= 0x07; /* 2 */
			    if (!(inport0 & 0x08)) data &= 0x0C; /* 3 */
			    if (!(inport0 & 0x10)) data &= 0x02; /* 4 */
			    if (!(inport0 & 0x20)) data &= 0x03; /* 5 */
			    if (!(inport0 & 0x40)) data &= 0x0E; /* 6 */
			    if (!(inport0 & 0x80)) data &= 0x05; /* 7 */
			    if (!(inport1 & 0x01)) data &= 0x01; /* 8 */
			    if (!(inport1 & 0x02)) data &= 0x0B; /* 9 */
			    if (!(inport1 & 0x04)) data &= 0x06; /* # */
			    if (!(inport1 & 0x08)) data &= 0x09; /* . */
			}
			return (inport1 & 0x70) | (data);
		}
		/* Joystick and fire 2 (SAC Red Button) */
		else
		{
			int data = input_port_read(space->machine, "controller1_joystick") & 0xCF;
			int inport6 = input_port_read(space->machine, "controllers");

			if (inport6&0x07) /* If Extra Contollers enabled */
			{
			    if (state->joy_stat[0]==0)
					data |= 0x30; /* Spinner Move Left */
			    else if (state->joy_stat[0]==1)
					data |= 0x20; /* Spinner Move Right */
			}
			return data | 0x80;
		}
	}
	/* Player 2 */
	else
	{
		/* Keypad and fire 1 */
		if (state->joy_mode==0)
		{
			int inport3,inport4,data;

			inport3 = input_port_read(space->machine, "controller2_keypad1");
			inport4 = input_port_read(space->machine, "controller2_keypad2");

			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
            than one, a real ColecoVision think that it is a third button, so we are going to emulate
            the right behaviour */

			data = 0x0F;	/* No key pressed by default */
			if (!(inport3 & 0x01)) data &= 0x0A; /* 0 */
			if (!(inport3 & 0x02)) data &= 0x0D; /* 1 */
			if (!(inport3 & 0x04)) data &= 0x07; /* 2 */
			if (!(inport3 & 0x08)) data &= 0x0C; /* 3 */
			if (!(inport3 & 0x10)) data &= 0x02; /* 4 */
			if (!(inport3 & 0x20)) data &= 0x03; /* 5 */
			if (!(inport3 & 0x40)) data &= 0x0E; /* 6 */
			if (!(inport3 & 0x80)) data &= 0x05; /* 7 */
			if (!(inport4 & 0x01)) data &= 0x01; /* 8 */
			if (!(inport4 & 0x02)) data &= 0x0B; /* 9 */
			if (!(inport4 & 0x04)) data &= 0x06; /* # */
			if (!(inport4 & 0x08)) data &= 0x09; /* . */

			return (inport4 & 0x70) | (data);
		}
		/* Joystick and fire 2*/
		else
		{
			int data = input_port_read(space->machine, "controller2_joystick") & 0xCF;
			int inport6 = input_port_read(space->machine, "controllers");

			if (inport6&0x02) /* If Roller Controller enabled */
			{
			    if (state->joy_stat[1]==0) data|=0x30;
			    else if (state->joy_stat[1]==1) data|=0x20;
			}

			return data | 0x80;
		}
	}

}


WRITE8_HANDLER ( adam_paddle_toggle_off )
{
	adam_state *state = space->machine->driver_data<adam_state>();
	state->joy_mode = 0;
}

WRITE8_HANDLER ( adam_paddle_toggle_on )
{
	adam_state *state = space->machine->driver_data<adam_state>();
	state->joy_mode = 1;
}
