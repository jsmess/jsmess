
/*

RM 380Z video code

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "imagedev/flopdrv.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "includes/rm380z.h"


UINT8 rm380z_state::decode_videoram_char(UINT8 ch1,UINT8 ch2)
{
	if ((ch1>0x0f)&&(ch1<0x80))
	{
		return ch1;
	}
	else if ((ch2>0x0f)&&(ch2<0x80))
	{
		return ch2;
	}
	else if ( ((ch1==0x04)&&(ch2==0x80)) || ((ch1==0x80)&&(ch2==0x04)) )
	{
		// blank out ?
		return 0x80;
	}
//  else if (ch1==0x80)
//  {
//      // blank out
//      return 0x20;
//  }
	else if ((ch1==0)&&(ch2==8))
	{
		// cursor
		return 0x7f;
	}
	else if ((ch1==0)&&(ch2==0))
	{
		// delete char (?)
		return 0x20;
	}
	else if ((ch1==4)&&(ch2==4))
	{
		// reversed cursor?
		return 0x20;
	}
	else if ((ch1==4)&&(ch2==8))
	{
		// normal cursor
		return 0x7f;
	}
	else
	{
		//printf("unhandled character combination [%x][%x]\n",ch1,ch2);
	}

	return 0;
}

// after ctrl-L (clear screen?): routine at EBBD is executed
// EB30??? next line?
// memory at FF02 seems to hold the line counter (same as FBFD)

WRITE8_MEMBER( rm380z_state::videoram_write )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	//printf("vramw [%x][%x] port0 [%x] fbfd [%x] PC [%x]\n",offset,data,state->m_port0,m_fbfd,cpu_get_pc(machine().device("maincpu")));

	if (m_old_fbfd>m_fbfd)
	{
		printf("WARN: fbfd wrapped\n");
	}
	m_old_fbfd=m_fbfd;

	int lineAdder=(m_fbfd&0x1f)*0x40;
	int realA=offset+lineAdder;

	// we suppose videoram is being written as character/attribute couple
	// fbfc 6th bit set=attribute, unset=char

	if (!(state->m_port0&0x40))
	{
		m_vramchars[realA%0x600]=data;
	}
	else
	{
		m_vramattribs[realA%0x600]=data;
	}

	UINT8 curch=decode_videoram_char(m_vramchars[realA],m_vramattribs[realA]);
	state->m_vram[realA%0x600]=curch;

	state->m_mainVideoram[offset]=data;
}

READ8_MEMBER( rm380z_state::videoram_read )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();
	//printf("read from videoram at [%x]\n",offset);
	//return state->m_vram[offset];
	return state->m_mainVideoram[offset];
}

static void putChar(int charnum,int x,int y,UINT16* pscr,unsigned char* chsb)
{
	if ((charnum>0)&&(charnum<=0x7f))
	{
		int basex=chdimx*(charnum/ncy);
		int basey=chdimy*(charnum%ncy);

		int inix=x*chdimx;
		int iniy=y*chdimx*screencols*chdimy;

		for (int r=0;r<chdimy;r++)
		{
			for (int c=0;c<chdimx;c++)
			{
				pscr[(inix+c)+(iniy+(r*chdimx*screencols))]=(chsb[((basey+r)*(chdimx*ncx))+(basex+c)])==0xff?0:1;
			}
		}
	}
}

void rm380z_state::update_screen(bitmap_t &bitmap)
{
	unsigned char* pChar=machine().region("gfx")->base();
	UINT16* scrptr = &bitmap.pix16(0);

	for (int row=0;row<screenrows;row++)
	{
		for (int col=0;col<screencols;col++)
		{
			unsigned char curchar=m_vram[(row*64)+col];
			putChar(curchar,col,row,scrptr,pChar);
		}
	}
}
