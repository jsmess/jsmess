/***************************************************************************

  adam.c

  Machine file to handle emulation of the ColecoAdam.

***************************************************************************/

#include "driver.h"
#include "video/tms9928a.h"
#include "includes/adam.h"
#include "devices/cartslot.h"
#include "image.h"
#include "devices/basicdsk.h"

int adam_lower_memory; /* Lower 32k memory Z80 address configuration */
int adam_upper_memory; /* Upper 32k memory Z80 address configuration */
int adam_joy_stat[2];
int adam_net_data; /* Data on AdamNet Bus */
int adam_pcb;

static int joy_mode = 0;
static int KeyboardBuffer[20]; /* Buffer to store keys pressed */
static int KbBufferCapacity = 15; /* Capacity of the Keyboard Buffer */
static int KbRepeatDelay = 20;
static UINT8 KbRepeatTable[256];

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

DEVICE_LOAD( adam_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* img, tracks, sides, sectors_per_track, sector_length, first_sector_id, offset_track_zero, track_skipping */
		basicdsk_set_geometry(image, 40, 1, 8, 512, 0, 0, FALSE);
		return INIT_PASS;
	}
	else
	{
		return INIT_FAIL;
	}
}



DEVICE_UNLOAD( adam_floppy )
{
    device_unload_basicdsk_floppy(image);
}



void clear_keyboard_buffer(void)
{
    int i;

    KeyboardBuffer[0] = 0; /* Keys on current buffer */
    for(i=1;i<KbBufferCapacity;i++)
    {
        KeyboardBuffer[i] = 0; /* Key pressed */
    }
    for(i=0;i<255;i++)
    {
        KbRepeatTable[i]=0; /* Clear Key Repeat Table */
    }

}

unsigned char getKeyFromBuffer(void)
{
	int i;
	UINT8 kbcode;

	kbcode=KeyboardBuffer[1];
	for(i=1;i<KeyboardBuffer[0];i++)
	{
		KeyboardBuffer[i]=KeyboardBuffer[i+1];
	}
	KeyboardBuffer[KeyboardBuffer[0]]=0;
	if (KeyboardBuffer[0]>0) KeyboardBuffer[0]--;
	return kbcode;
}

void addToKeyboardBuffer(unsigned char kbcode)
{
    if (KbRepeatTable[kbcode]==0)
    {
        if ((KeyboardBuffer[0] < KbBufferCapacity))
        {
            KeyboardBuffer[0]++;
            KeyboardBuffer[KeyboardBuffer[0]] = kbcode;
            KbRepeatTable[kbcode]=255-KbRepeatDelay;
        }
    }
}

void exploreKeyboard(void)
{
    bool controlKey, shiftKey;
    int i, keyboard[10];

//logerror("Exploring Keyboard.............\n");
    for(i=0;i<=255;i++) {if (KbRepeatTable[i]>0) KbRepeatTable[i]++;} /* Update repeat table */

    keyboard[0] = input_port_9_r(0);
    keyboard[1] = input_port_10_r(0);
    keyboard[2] = input_port_11_r(0);
    keyboard[3] = input_port_12_r(0);
    keyboard[4] = input_port_13_r(0);
    keyboard[5] = input_port_14_r(0);
    keyboard[6] = input_port_15_r(0);
    keyboard[7] = input_port_16_r(0);
    keyboard[8] = input_port_17_r(0);
    keyboard[9] = input_port_18_r(0);
    
    controlKey = (!(keyboard[9]&0x01) || !(keyboard[9]&0x02)); /* Left Control or Right Control */
    shiftKey = (!(keyboard[8]&0x40) || !(keyboard[8]&0x80) || !(keyboard[9]&0x04)); /* Left shiftKey or Right shiftKey or Caps Lock*/

    if (shiftKey && !controlKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(0xB8); /* shiftKey + BACKSPACE */
    else {KbRepeatTable[0xB8]=0;}
    
    if (shiftKey && !controlKey && !(keyboard[8] & 0x20)) addToKeyboardBuffer(0xB9); /* shiftKey + TAB */
    else {KbRepeatTable[0xB9]=0;}
    
    if (shiftKey && !controlKey && !(keyboard[4] & 0x01)) addToKeyboardBuffer(0x29); /* shiftKey + 0 */
    else {KbRepeatTable[0x29]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x02)) addToKeyboardBuffer(0x21); /* shiftKey + 1 */
    else {KbRepeatTable[0x21]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(0x40); /* shiftKey + 2 */
    else {KbRepeatTable[0x40]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x08)) addToKeyboardBuffer(0x23); /* shiftKey + 3 */
    else {KbRepeatTable[0x23]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x10)) addToKeyboardBuffer(0x24); /* shiftKey + 4 */
    else {KbRepeatTable[0x24]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x20)) addToKeyboardBuffer(0x25); /* shiftKey + 5 */
    else {KbRepeatTable[0x25]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(0x5F); /* shiftKey + 6 */
    else {KbRepeatTable[0x5F]=0;}
    if (shiftKey && !controlKey && !(keyboard[4] & 0x80)) addToKeyboardBuffer(0x26); /* shiftKey + 7 */
    else {KbRepeatTable[0x26]=0;}

    if (shiftKey && !controlKey && !(keyboard[5] & 0x01)) addToKeyboardBuffer(0x2A); /* shiftKey + 8 */
    else {KbRepeatTable[0x2A]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x02)) addToKeyboardBuffer(0x28); /* shiftKey + 9 */
    else {KbRepeatTable[0x28]=0;}

    if (!shiftKey && !controlKey && !(keyboard[4] & 0x01)) addToKeyboardBuffer(0x30); /* 0 */
    else {KbRepeatTable[0x30]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x02)) addToKeyboardBuffer(0x31); /* 1 */
    else {KbRepeatTable[0x31]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(0x32); /* 2 */
    else {KbRepeatTable[0x32]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x08)) addToKeyboardBuffer(0x33); /* 3 */
    else {KbRepeatTable[0x33]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x10)) addToKeyboardBuffer(0x34); /* 4 */
    else {KbRepeatTable[0x34]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x20)) addToKeyboardBuffer(0x35); /* 5 */
    else {KbRepeatTable[0x35]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(0x36); /* 6 */
    else {KbRepeatTable[0x36]=0;}
    if (!shiftKey && !controlKey && !(keyboard[4] & 0x80)) addToKeyboardBuffer(0x37); /* 7 */
    else {KbRepeatTable[0x37]=0;}

    if (!shiftKey && !controlKey && !(keyboard[5] & 0x01)) addToKeyboardBuffer(0x38); /* 8 */
    else {KbRepeatTable[0x38]=0;}
    if (!shiftKey && !controlKey && !(keyboard[5] & 0x02)) addToKeyboardBuffer(0x39); /* 9 */
    else {KbRepeatTable[0x39]=0;}

    if (controlKey && !shiftKey && !(keyboard[4] & 0x04)) addToKeyboardBuffer(0x00); /* controlKey + 2 */
    else {KbRepeatTable[0x00]=0;}
    if (controlKey && !shiftKey && !(keyboard[4] & 0x40)) addToKeyboardBuffer(0x1F); /* controlKey + 6 */
    else {KbRepeatTable[0x1F]=0;}


    if (shiftKey && !controlKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(0x41); /* shiftKey + A */
    else {KbRepeatTable[0x41]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(0x42); /* shiftKey + B */
    else {KbRepeatTable[0x42]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(0x43); /* shiftKey + C */
    else {KbRepeatTable[0x43]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(0x44); /* shiftKey + D */
    else {KbRepeatTable[0x44]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(0x45); /* shiftKey + E */
    else {KbRepeatTable[0x45]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(0x46); /* shiftKey + F */
    else {KbRepeatTable[0x46]=0;}
    if (shiftKey && !controlKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(0x47); /* shiftKey + G */
    else {KbRepeatTable[0x47]=0;}

    if (shiftKey && !controlKey && !(keyboard[1] & 0x01)) addToKeyboardBuffer(0x48); /* shiftKey + H */
    else {KbRepeatTable[0x48]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x02)) addToKeyboardBuffer(0x49); /* shiftKey + I */
    else {KbRepeatTable[0x49]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(0x4A); /* shiftKey + J */
    else {KbRepeatTable[0x4A]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(0x4B); /* shiftKey + K */
    else {KbRepeatTable[0x4B]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(0x4C); /* shiftKey + L */
    else {KbRepeatTable[0x4C]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x20)) addToKeyboardBuffer(0x4D); /* shiftKey + M */
    else {KbRepeatTable[0x4D]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(0x4E); /* shiftKey + N */
    else {KbRepeatTable[0x4E]=0;}
    if (shiftKey && !controlKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(0x4F); /* shiftKey + O */
    else {KbRepeatTable[0x4F]=0;}

    if (shiftKey && !controlKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(0x50); /* shiftKey + P */
    else {KbRepeatTable[0x50]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(0x51); /* shiftKey + Q */
    else {KbRepeatTable[0x51]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(0x52); /* shiftKey + R */
    else {KbRepeatTable[0x52]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(0x53); /* shiftKey + S */
    else {KbRepeatTable[0x53]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(0x54); /* shiftKey + T */
    else {KbRepeatTable[0x54]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(0x55); /* shiftKey + U */
    else {KbRepeatTable[0x55]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(0x56); /* shiftKey + V */
    else {KbRepeatTable[0x56]=0;}
    if (shiftKey && !controlKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(0x57); /* shiftKey + W */
    else {KbRepeatTable[0x57]=0;}

    if (shiftKey && !controlKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(0x58); /* shiftKey + X */
    else {KbRepeatTable[0x58]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(0x59); /* shiftKey + Y */
    else {KbRepeatTable[0x59]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(0x5A); /* shiftKey + Z */
    else {KbRepeatTable[0x5A]=0;}

    if (controlKey && !shiftKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(0x01); /* controlKey + A */
    else {KbRepeatTable[0x01]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(0x02); /* controlKey + B */
    else {KbRepeatTable[0x02]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(0x03); /* controlKey + C */
    else {KbRepeatTable[0x03]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(0x04); /* controlKey + D */
    else {KbRepeatTable[0x04]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(0x05); /* controlKey + E */
    else {KbRepeatTable[0x05]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(0x06); /* controlKey + F */
    else {KbRepeatTable[0x06]=0;}
    if (controlKey && !shiftKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(0x07); /* controlKey + G */
    else {KbRepeatTable[0x07]=0;}

    if ((controlKey && !shiftKey && !(keyboard[1] & 0x01)) || !(keyboard[7] & 0x80)) addToKeyboardBuffer(0x08); /* controlKey + H or BACKSPACE */
    else {KbRepeatTable[0x08]=0;}
    if ((controlKey && !shiftKey && !(keyboard[1] & 0x02)) || !(keyboard[8] & 0x20)) addToKeyboardBuffer(0x09); /* controlKey + I or TAB */
    else {KbRepeatTable[0x09]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(0x0A); /* controlKey + J */
    else {KbRepeatTable[0x0A]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(0x0B); /* controlKey + K */
    else {KbRepeatTable[0x0B]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(0x0C); /* controlKey + L */
    else {KbRepeatTable[0x0C]=0;}
    if ((controlKey && !shiftKey && !(keyboard[1] & 0x20)) || !(keyboard[6] & 0x80)) addToKeyboardBuffer(0x0D); /* controlKey + M or RETURN */
    else {KbRepeatTable[0x0D]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(0x0E); /* controlKey + N */
    else {KbRepeatTable[0x0E]=0;}
    if (controlKey && !shiftKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(0x0F); /* controlKey + O */
    else {KbRepeatTable[0x0F]=0;}
    
    if (controlKey && !shiftKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(0x10); /* controlKey + P */
    else {KbRepeatTable[0x10]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(0x11); /* controlKey + Q */
    else {KbRepeatTable[0x11]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(0x12); /* controlKey + R */
    else {KbRepeatTable[0x12]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(0x13); /* controlKey + S */
    else {KbRepeatTable[0x13]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(0x14); /* controlKey + T */
    else {KbRepeatTable[0x14]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(0x15); /* controlKey + U */
    else {KbRepeatTable[0x15]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(0x16); /* controlKey + V */
    else {KbRepeatTable[0x16]=0;}
    if (controlKey && !shiftKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(0x17); /* controlKey + W */
    else {KbRepeatTable[0x17]=0;}

    if (controlKey && !shiftKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(0x18); /* controlKey + X */
    else {KbRepeatTable[0x18]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(0x19); /* controlKey + Y */
    else {KbRepeatTable[0x19]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(0x1A); /* controlKey + Z */
    else {KbRepeatTable[0x1A]=0;}

    if (!controlKey && !shiftKey && !(keyboard[0] & 0x02)) addToKeyboardBuffer(0x61); /* A */
    else {KbRepeatTable[0x61]=0;}

    if (!controlKey && !shiftKey && !(keyboard[0] & 0x04)) addToKeyboardBuffer(0x62); /* B */
    else {KbRepeatTable[0x62]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x08)) addToKeyboardBuffer(0x63); /* C */
    else {KbRepeatTable[0x63]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x10)) addToKeyboardBuffer(0x64); /* D */
    else {KbRepeatTable[0x64]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x20)) addToKeyboardBuffer(0x65); /* E */
    else {KbRepeatTable[0x65]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x40)) addToKeyboardBuffer(0x66); /* F */
    else {KbRepeatTable[0x66]=0;}
    if (!controlKey && !shiftKey && !(keyboard[0] & 0x80)) addToKeyboardBuffer(0x67); /* G */
    else {KbRepeatTable[0x67]=0;}

    if (!controlKey && !shiftKey && !(keyboard[1] & 0x01)) addToKeyboardBuffer(0x68); /* H */
    else {KbRepeatTable[0x68]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x02)) addToKeyboardBuffer(0x69); /* I */
    else {KbRepeatTable[0x69]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x04)) addToKeyboardBuffer(0x6A); /* J */
    else {KbRepeatTable[0x6A]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x08)) addToKeyboardBuffer(0x6B); /* K */
    else {KbRepeatTable[0x6B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x10)) addToKeyboardBuffer(0x6C); /* L */
    else {KbRepeatTable[0x6C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x20)) addToKeyboardBuffer(0x6D); /* M */
    else {KbRepeatTable[0x6D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x40)) addToKeyboardBuffer(0x6E); /* N */
    else {KbRepeatTable[0x6E]=0;}
    if (!controlKey && !shiftKey && !(keyboard[1] & 0x80)) addToKeyboardBuffer(0x6F); /* O */
    else {KbRepeatTable[0x6F]=0;}

    if (!controlKey && !shiftKey && !(keyboard[2] & 0x01)) addToKeyboardBuffer(0x70); /* P */
    else {KbRepeatTable[0x70]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x02)) addToKeyboardBuffer(0x71); /* Q */
    else {KbRepeatTable[0x71]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x04)) addToKeyboardBuffer(0x72); /* R */
    else {KbRepeatTable[0x72]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x08)) addToKeyboardBuffer(0x73); /* S */
    else {KbRepeatTable[0x73]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x10)) addToKeyboardBuffer(0x74); /* T */
    else {KbRepeatTable[0x74]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x20)) addToKeyboardBuffer(0x75); /* U */
    else {KbRepeatTable[0x75]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x40)) addToKeyboardBuffer(0x76); /* V */
    else {KbRepeatTable[0x76]=0;}
    if (!controlKey && !shiftKey && !(keyboard[2] & 0x80)) addToKeyboardBuffer(0x77); /* W */
    else {KbRepeatTable[0x77]=0;}

    if (!controlKey && !shiftKey && !(keyboard[3] & 0x01)) addToKeyboardBuffer(0x78); /* X */
    else {KbRepeatTable[0x78]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x02)) addToKeyboardBuffer(0x79); /* Y */
    else {KbRepeatTable[0x79]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x04)) addToKeyboardBuffer(0x7A); /* Z */
    else {KbRepeatTable[0x7A]=0;}



    if (shiftKey && !controlKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(0x22); /* shiftKey + ' *//****** Not sure ******/
    else {KbRepeatTable[0x22]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x80)) addToKeyboardBuffer(0x3A); /* shiftKey + ; */    
    else {KbRepeatTable[0x3A]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x10)) addToKeyboardBuffer(0x3C); /* shiftKey + , */
    else {KbRepeatTable[0x3C]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x08)) addToKeyboardBuffer(0x3D); /* shiftKey + + */
    else {KbRepeatTable[0x3D]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x20)) addToKeyboardBuffer(0x3E); /* shiftKey + . */
    else {KbRepeatTable[0x3E]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x40)) addToKeyboardBuffer(0x3F); /* shiftKey + / */
    else {KbRepeatTable[0x3F]=0;}

    if (shiftKey && !controlKey && !(keyboard[3] & 0x40)) addToKeyboardBuffer(0x60); /* shiftKey + - */
    else {KbRepeatTable[0x60]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x80)) addToKeyboardBuffer(0x7B); /* shiftKey + [ */
    else {KbRepeatTable[0x7B]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(0x7C); /* shiftKey + \ *//****** Not sure  ******/
    else {KbRepeatTable[0x7C]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(0x7D); /* shiftKey + ] */
    else {KbRepeatTable[0x7D]=0;}
    if (shiftKey && !controlKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(0x7E); /* shiftKey + ^ */
    else {KbRepeatTable[0x7E]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x01)) addToKeyboardBuffer(0x89); /* shiftKey + I */
    else {KbRepeatTable[0x89]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x02)) addToKeyboardBuffer(0x8A); /* shiftKey + II */
    else {KbRepeatTable[0x8A]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x04)) addToKeyboardBuffer(0x8B); /* shiftKey + III */
    else {KbRepeatTable[0x8B]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x08)) addToKeyboardBuffer(0x8C); /* shiftKey + IV */
    else {KbRepeatTable[0x8C]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x10)) addToKeyboardBuffer(0x8D); /* shiftKey + V */
    else {KbRepeatTable[0x8D]=0;}
    if (shiftKey && !controlKey && !(keyboard[6] & 0x20)) addToKeyboardBuffer(0x8E); /* shiftKey + VI */
    else {KbRepeatTable[0x8E]=0;}

    if (shiftKey && !controlKey && !(keyboard[7] & 0x01)) addToKeyboardBuffer(0x98); /* shiftKey + WILD CARD */
    else {KbRepeatTable[0x98]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x02)) addToKeyboardBuffer(0x99); /* shiftKey + UNDO */
    else {KbRepeatTable[0x99]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x04)) addToKeyboardBuffer(0x9A); /* shiftKey + MOVE */
    else {KbRepeatTable[0x9A]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x08)) addToKeyboardBuffer(0x9B); /* shiftKey + STORE */
    else {KbRepeatTable[0x9B]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x10)) addToKeyboardBuffer(0x9C); /* shiftKey + INSERT */
    else {KbRepeatTable[0x9C]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x20)) addToKeyboardBuffer(0x9D); /* shiftKey + PRINT */
    else {KbRepeatTable[0x9D]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x40)) addToKeyboardBuffer(0x9E); /* shiftKey + CLEAR */
    else {KbRepeatTable[0x9E]=0;}
    if (shiftKey && !controlKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(0x9F); /* shiftKey + DELETE */
    else {KbRepeatTable[0x9F]=0;}



    if (controlKey && !shiftKey && !(keyboard[8] & 0x02)) addToKeyboardBuffer(0xA4); /* controlKey + UP ARROW */
    else {KbRepeatTable[0xA4]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x10)) addToKeyboardBuffer(0xA5); /* controlKey + RIGHT ARROW */
    else {KbRepeatTable[0xA5]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x04)) addToKeyboardBuffer(0xA6); /* controlKey + DOWN ARROW */
    else {KbRepeatTable[0xA6]=0;}
    if (controlKey && !shiftKey && !(keyboard[8] & 0x08)) addToKeyboardBuffer(0xA7); /* controlKey + LEFT ARROW */
    else {KbRepeatTable[0xA7]=0;}

    if (controlKey && !shiftKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(0x7F); /* controlKey + DELETE */
    else {KbRepeatTable[0x7F]=0;}




    if ((controlKey && !shiftKey && !(keyboard[5] & 0x80)) || !(keyboard[0] & 0x01)) addToKeyboardBuffer(0x1B); /* controlKey + [ or WP/ESCAPE */
    else {KbRepeatTable[0x1B]=0;}
    if (controlKey && !shiftKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(0x1C); /* controlKey + \ *//****** Not sure ******/
    else {KbRepeatTable[0x1C]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(0x1D); /* controlKey + ] */
    else {KbRepeatTable[0x1D]=0;}
    if (controlKey && !shiftKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(0x1E); /* controlKey + ^ */
    else {KbRepeatTable[0x1E]=0;}

    
    if (!controlKey && !shiftKey && !(keyboard[6] & 0x40)) addToKeyboardBuffer(0x20); /* SPACEBAR */
    else {KbRepeatTable[0x20]=0;}
    if (shiftKey && !controlKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(0x27); /* ' */
    else {KbRepeatTable[0x27]=0;}

    if (!controlKey && !shiftKey && !(keyboard[5] & 0x08)) addToKeyboardBuffer(0x2B); /* + */
    else {KbRepeatTable[0x2B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x10)) addToKeyboardBuffer(0x2C); /* , */
    else {KbRepeatTable[0x2C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x40)) addToKeyboardBuffer(0x2D); /* - */
    else {KbRepeatTable[0x2D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x20)) addToKeyboardBuffer(0x2E); /* . */
    else {KbRepeatTable[0x2E]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x40)) addToKeyboardBuffer(0x2F); /* / */
    else {KbRepeatTable[0x2F]=0;}
    

    if (!controlKey && !shiftKey && !(keyboard[3] & 0x80)) addToKeyboardBuffer(0x3B); /* ; */
    else {KbRepeatTable[0x3B]=0;}


    if (!controlKey && !shiftKey && !(keyboard[5] & 0x80)) addToKeyboardBuffer(0x5B); /* [ */
    else {KbRepeatTable[0x5B]=0;}
    if (!controlKey && !shiftKey && !(keyboard[5] & 0x04)) addToKeyboardBuffer(0x5C); /* \ *//****** Not sure ******/
    else {KbRepeatTable[0x5C]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x08)) addToKeyboardBuffer(0x5D); /* ] */
    else {KbRepeatTable[0x5D]=0;}
    if (!controlKey && !shiftKey && !(keyboard[3] & 0x20)) addToKeyboardBuffer(0x5E); /* ^ */
    else {KbRepeatTable[0x5E]=0;}


    if (!shiftKey && !(keyboard[8] & 0x01)) addToKeyboardBuffer(0x80); /* HOME */
    else {KbRepeatTable[0x80]=0;}
    if (!shiftKey && !(keyboard[6] & 0x01)) addToKeyboardBuffer(0x81); /* I */
    else {KbRepeatTable[0x81]=0;}
    if (!shiftKey && !(keyboard[6] & 0x02)) addToKeyboardBuffer(0x82); /* II */
    else {KbRepeatTable[0x82]=0;}
    if (!shiftKey && !(keyboard[6] & 0x04)) addToKeyboardBuffer(0x83); /* III */
    else {KbRepeatTable[0x83]=0;}
    if (!shiftKey && !(keyboard[6] & 0x08)) addToKeyboardBuffer(0x84); /* IV */
    else {KbRepeatTable[0x84]=0;}
    if (!shiftKey && !(keyboard[6] & 0x10)) addToKeyboardBuffer(0x85); /* V */
    else {KbRepeatTable[0x85]=0;}
    if (!shiftKey && !(keyboard[6] & 0x20)) addToKeyboardBuffer(0x86); /* VI */
    else {KbRepeatTable[0x86]=0;}

    if (!shiftKey && !(keyboard[7] & 0x01)) addToKeyboardBuffer(0x90); /* WILD CARD */
    else {KbRepeatTable[0x90]=0;}
    if (!shiftKey && !(keyboard[7] & 0x02)) addToKeyboardBuffer(0x91); /* UNDO */
    else {KbRepeatTable[0x91]=0;}
    if (!shiftKey && !(keyboard[7] & 0x04)) addToKeyboardBuffer(0x92); /* MOVE */
    else {KbRepeatTable[0x92]=0;}
    if (!shiftKey && !(keyboard[7] & 0x08)) addToKeyboardBuffer(0x93); /* STORE */
    else {KbRepeatTable[0x93]=0;}
    if (!shiftKey && !(keyboard[7] & 0x10)) addToKeyboardBuffer(0x94); /* INSERT */
    else {KbRepeatTable[0x94]=0;}
    if (!shiftKey && !(keyboard[7] & 0x20)) addToKeyboardBuffer(0x95); /* PRINT */
    else {KbRepeatTable[0x95]=0;}
    if (!shiftKey && !(keyboard[7] & 0x40)) addToKeyboardBuffer(0x96); /* CLEAR */
    else {KbRepeatTable[0x96]=0;}
    if (!shiftKey && !(keyboard[7] & 0x80)) addToKeyboardBuffer(0x97); /* DELETE */
    else {KbRepeatTable[0x97]=0;}
    

    if (!controlKey && !(keyboard[8] & 0x02)) addToKeyboardBuffer(0xA0); /* UP ARROW */
    else {KbRepeatTable[0xA0]=0;}
    if (!controlKey && !(keyboard[8] & 0x10)) addToKeyboardBuffer(0xA1); /* RIGHT ARROW */
    else {KbRepeatTable[0xA1]=0;}
    if (!controlKey && !(keyboard[8] & 0x04)) addToKeyboardBuffer(0xA2); /* DOWN ARROW */
    else {KbRepeatTable[0xA2]=0;}
    if (!controlKey && !(keyboard[8] & 0x08)) addToKeyboardBuffer(0xA3); /* LEFT ARROW */
    else {KbRepeatTable[0xA3]=0;}

    if (!controlKey && !(keyboard[8] & 0x02) && !(keyboard[8] & 0x10)) addToKeyboardBuffer(0xA8); /* UP ARROW + RIGHT ARROW */
    else {KbRepeatTable[0xA8]=0;}
    if (!controlKey && !(keyboard[8] & 0x10) && !(keyboard[8] & 0x04)) addToKeyboardBuffer(0xA9); /* RIGHT ARROW + DOWN ARROW */
    else {KbRepeatTable[0xA9]=0;}
    if (!controlKey && !(keyboard[8] & 0x04) && !(keyboard[8] & 0x08)) addToKeyboardBuffer(0xAA); /* DOWN ARROW + LEFT ARROW */
    else {KbRepeatTable[0xAA]=0;}
    if (!controlKey && !(keyboard[8] & 0x08) && !(keyboard[8] & 0x02)) addToKeyboardBuffer(0xAB); /* LEFT ARROW + UP ARROW */
    else {KbRepeatTable[0xAB]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x02)) addToKeyboardBuffer(0xAC); /* HOME + UP ARROW */
    else {KbRepeatTable[0xAC]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x10)) addToKeyboardBuffer(0xAD); /* HOME + RIGHT ARROW */
    else {KbRepeatTable[0xAD]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x04)) addToKeyboardBuffer(0xAE); /* HOME + DOWN ARROW */
    else {KbRepeatTable[0xAE]=0;}
    if (!controlKey && !(keyboard[8] & 0x01) && !(keyboard[8] & 0x08)) addToKeyboardBuffer(0xAF); /* HOME + LEFT ARROW */
    else {KbRepeatTable[0xAF]=0;}

}

void master6801_behaviour(int offset, int data)
{
/*
The Master MC6801 controls all AdamNet operations, keyboard, tapes, disks...
We are going to "simulate" the behaviour of the master 6801, because we does not have the 2kbytes ROM DUMP
If you have the source listing or the Rom dump, please send us.
*/
	int DCB_Num, deviceNum, statusDCB;
	int i, buffer, byteCount, sectorNmbr, sectorCount, currentSector;
	UINT8 kbcode;
	static UINT8 interleave[8] = {0,5,2,7,4,1,6,3};
	mess_image *image;

	if (offset == adam_pcb)
	{
		switch (data)
		{
			case 1: /* Request to synchronize the Z80 clock */
				memory_region(REGION_CPU1)[offset] = 0x81;
				//logerror("Synchronizing the Z80 clock\n");
				break;
			case 2: /* Request to synchronize the Master 6801 clock */
				memory_region(REGION_CPU1)[offset] = 0x82;
				//logerror("Synchronizing the Master 6801 clock\n");
				break;
			case 3: /* Request to relocate adam_pcb */
				memory_region(REGION_CPU1)[offset] = 0x83; /* Must return 0x83 if success */
				//memory_region(REGION_CPU1)[offset] = 0x9B; /* Time Out */
				logerror("Request to relocate adam_pcb from %04Xh to %04Xh... not implemented... but returns OK\n", adam_pcb, (memory_region(REGION_CPU1)[adam_pcb+1]&memory_region(REGION_CPU1)[adam_pcb+2]<<8));
				break;
		}
	}

	for(DCB_Num=1;DCB_Num<=memory_region(REGION_CPU1)[adam_pcb+3];DCB_Num++) /* Test status of each DCB in adam_pcb table */
	{
		statusDCB = (adam_pcb+4)+(DCB_Num-1)*21;
		if (offset==statusDCB)
		{
			//logerror("Accesing DCB %02Xh\n", DCB_Num);
			deviceNum = (memory_region(REGION_CPU1)[statusDCB+0x10]&0x0F)+(memory_region(REGION_CPU1)[statusDCB+0x09]<<4);
			buffer=(memory_region(REGION_CPU1)[statusDCB+0x01])+(memory_region(REGION_CPU1)[statusDCB+0x02]<<8);
			byteCount=(memory_region(REGION_CPU1)[statusDCB+0x03])+(memory_region(REGION_CPU1)[statusDCB+0x04]<<8);

			if (deviceNum>=4 && deviceNum<=7) 
			{
				image = image_from_devtype_and_index(IO_FLOPPY, deviceNum - 4);
				if (image_exists(image))
					memory_region(REGION_CPU1)[statusDCB+20] = (memory_region(REGION_CPU1)[statusDCB+20]&0xF0); /* Inserted Media */
				else
					memory_region(REGION_CPU1)[statusDCB+20] = (memory_region(REGION_CPU1)[statusDCB+20]&0xF0)|0x03; /* No Media on Drive*/
			}
			switch (data)
			{
				case 0: /* Initialize Drive */
					if (deviceNum>=4 && deviceNum<=7)
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x80;
					}
					break;
				case 1: /* Return current status */
					if (deviceNum==1||deviceNum==2)
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x80; /* Device Found (1=Keyboard, 2=Printer) */
						memory_region(REGION_CPU1)[statusDCB+0x13] = 0x01; /* Character device */
					}
					else if (deviceNum>=4 && deviceNum<=7)
					{
						image = image_from_devtype_and_index(IO_FLOPPY, deviceNum - 4);
						if (image_exists(image))
						{
							memory_region(REGION_CPU1)[statusDCB] = 0x80;
							memory_region(REGION_CPU1)[statusDCB+17] = 1024&255;
							memory_region(REGION_CPU1)[statusDCB+18] = 1024>>8;
						}
						else
						{
							memory_region(REGION_CPU1)[statusDCB] = 0x83; /* Device Found but No Disk in Drive*/
						}
					}
					else
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x9B; /* Time Out - No Device Found*/
					}
					//logerror("   Requesting Status Device %02d=%02Xh\n", deviceNum, memory_region(REGION_CPU1)[statusDCB]);
					break;
				case 2: /* Soft reset */
					memory_region(REGION_CPU1)[statusDCB] = 0x80;
					//logerror("   Reseting Device %02d\n", deviceNum);
					break;
				case 3: /* Write Data */
					//logerror("   Requesting Write to Device %02d\n", deviceNum);
					if (deviceNum==2)
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x80; /* Ok, char sent to printer... no really */
						//logerror("   Requesting Write %2d bytes on buffer [%04xh] to Device %02d\n", byteCount, buffer,deviceNum);
					}
					else
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x85; /* Write Protected Media */
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
						kbcode=getKeyFromBuffer();
						if (kbcode>0)
						{
							memory_region(REGION_CPU1)[buffer] = kbcode; /* Key pressed  */
							memory_region(REGION_CPU1)[statusDCB] = 0x80;
						}
						else
						{
							memory_region(REGION_CPU1)[statusDCB] = 0x8C; /* No key pressed */
						}
					}
					else if (deviceNum>=4 && deviceNum<=7)
					{
						image = image_from_devtype_and_index(IO_FLOPPY, deviceNum - 4);
						if (image_exists(image))
						{
							sectorNmbr = ((memory_region(REGION_CPU1)[statusDCB+5])+(memory_region(REGION_CPU1)[statusDCB+6]<<8)+(memory_region(REGION_CPU1)[statusDCB+7]<<16)+(memory_region(REGION_CPU1)[statusDCB+8]<<24))<<1;
							sectorCount = (byteCount/512)+(byteCount%512==0)? 0:1;
							for(i=0;i<=1;i++)
							{
								currentSector = floppy_drive_get_current_track(image);
								while (currentSector > ((sectorNmbr+i)/8))
								{
									floppy_drive_seek(image, -1);
									currentSector--;
								}
								while (currentSector < ((sectorNmbr+i)/8))
								{
									floppy_drive_seek(image, 1);
									currentSector++;
								}
								floppy_drive_read_sector_data(image, 0, interleave[(sectorNmbr+i)&0x07], &memory_region(REGION_CPU1)[buffer+(512*i)],512);
							}
							memory_region(REGION_CPU1)[statusDCB+20] |= 6;
							memory_region(REGION_CPU1)[statusDCB] = 0x80;
						}

					}
					else
					{
						memory_region(REGION_CPU1)[statusDCB] = 0x9B;
					}
					//logerror("   Requesting Read %2d bytes on buffer [%04xh] from Device %02d\n", byteCount, buffer,deviceNum);
					break;
				default:
					memory_region(REGION_CPU1)[statusDCB] = 0x9B; /* Other */
					break;
			}
		}
	}
}

WRITE8_HANDLER( common_writes_w )
{
    switch (adam_upper_memory)
    {
        case 0: /* Internal RAM */
            memory_region(REGION_CPU1)[0x08000+offset] = data;
            if (offset>=(adam_pcb-0x08000)) master6801_behaviour(offset+0x08000, data);
            break;
        case 1: /* ROM Expansion */
            break;
        case 2: /* RAM Expansion */
            memory_region(REGION_CPU1)[0x18000+offset] = data;
        	break;
    	case 3: /* Cartridge ROM */
        	break;
    }
}

 READ8_HANDLER( adamnet_r )
{  
    //logerror("adam_net_data Read %2xh\n",adam_net_data);
    return adam_net_data;
}

WRITE8_HANDLER( adamnet_w )
{
	/*
	If SmartWriter ROM is selected on lower Z80 memory 
	if data bit1 is 1 -> Lower 32k = EOS otherwise Lower 32k = SmartWriter
	*/
	UINT8 *BankBase;
	BankBase = &memory_region(REGION_CPU1)[0x00000];

	if (data==0x0F) resetPCB();
	if ( (adam_lower_memory==0) && ((data&0x02)!=(adam_net_data&0x02)) )
	{
		if (data&0x02)
		{
			memory_set_bankptr(1, BankBase+0x32000); /* No data here */
			memory_set_bankptr(2, BankBase+0x34000); /* No data here */
			memory_set_bankptr(3, BankBase+0x36000); /* No data here */
			memory_set_bankptr(4, BankBase+0x38000); /* EOS ROM */

			memory_set_bankptr(6, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(7, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(8, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(9, BankBase+0x3A000); /* Write protecting ROM */
		}
		else
		{
			memory_set_bankptr(1, BankBase+0x20000); /* SmartWriter ROM */
			memory_set_bankptr(2, BankBase+0x22000);
			memory_set_bankptr(3, BankBase+0x24000);
			memory_set_bankptr(4, BankBase+0x26000);
	        
			memory_set_bankptr(6, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(7, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(8, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(9, BankBase+0x3A000); /* Write protecting ROM */
		}
	}
	adam_net_data = data;
}

 READ8_HANDLER( adam_memory_map_controller_r )
{
    return (adam_upper_memory << 2) + adam_lower_memory;
}

WRITE8_HANDLER( adam_memory_map_controller_w )
{
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
    
    adam_lower_memory = (data & 0x03);
    adam_upper_memory = (data & 0x0C)>>2;
    set_memory_banks();
    //logerror("Configurando la memoria, L:%02xh, U:%02xh\n", adam_lower_memory, adam_upper_memory);
}

 READ8_HANDLER(adam_video_r)
{  
    return ((offset&0x01) ? TMS9928A_register_r(1) : TMS9928A_vram_r(0));
}

WRITE8_HANDLER(adam_video_w)
{
    (offset&0x01) ? TMS9928A_register_w(1, data) : TMS9928A_vram_w(0, data);
}

 READ8_HANDLER( master6801_ram_r )
{  
    /*logerror("Offset %04Xh = %02Xh\n",offset ,memory_region(REGION_CPU1)[offset]);*/
    return memory_region(REGION_CPU1)[offset+0x4000];
}

WRITE8_HANDLER( master6801_ram_w )
{
    memory_region(REGION_CPU1)[offset+0x4000] = data;
}

 READ8_HANDLER ( adam_paddle_r )
{

    if (!(input_port_6_r(0) & 0x0F)) return (0xFF); /* If no controllers enabled */
	/* Player 1 */
	if ((offset & 0x02)==0)
	{
		/* Keypad and fire 1 (SAC Yellow Button) */
		if (joy_mode==0)
		{
			int inport0,inport1,inport6,data;

			inport0 = input_port_0_r(0);
			inport1 = input_port_1_r(0);
			inport6 = input_port_6_r(0);
			
			/* Numeric pad buttons are not independent on a real ColecoVision, if you push more 
			than one, a real ColecoVision think that it is a third button, so we are going to emulate
			the right behaviour */
			
			data = 0x0F;	/* No key pressed by default */
			if (!(inport6&0x01)) /* If Driving Controller enabled -> no keypad 1*/
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
			int data = input_port_2_r(0) & 0xCF;
			int inport6 = input_port_6_r(0);
			
			if (inport6&0x07) /* If Extra Contollers enabled */
			{
			    if (adam_joy_stat[0]==0)
					data |= 0x30; /* Spinner Move Left*/
			    else if (adam_joy_stat[0]==1)
					data |= 0x20; /* Spinner Move Right */
			}  
			return data | 0x80;
		}
	}
	/* Player 2 */
	else
	{
		/* Keypad and fire 1 */
		if (joy_mode==0)
		{
			int inport3,inport4,data;

			inport3 = input_port_3_r(0);
			inport4 = input_port_4_r(0);

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
			int data = input_port_5_r(0) & 0xCF;
			int inport6 = input_port_6_r(0);
			
			if (inport6&0x02) /* If Roller Controller enabled */
			{
			    if (adam_joy_stat[1]==0) data|=0x30;
			    else if (adam_joy_stat[1]==1) data|=0x20;
			}  

			return data | 0x80;
		}
	}

}


WRITE8_HANDLER ( adam_paddle_toggle_off )
{
	joy_mode = 0;
    return;
}

WRITE8_HANDLER ( adam_paddle_toggle_on )
{
	joy_mode = 1;
    return;
}
