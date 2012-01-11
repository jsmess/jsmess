
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


void rm380z_state::config_videomode()
{
	if (m_port0&0x20)
	{
		// 80 cols
		m_videomode=RM380Z_VIDEOMODE_80COL;
	}
	else 
	{
		// 40 cols
		m_videomode=RM380Z_VIDEOMODE_40COL;
	}

	if (m_old_videomode!=m_videomode)
	{
		m_old_videomode=m_videomode;
		memset(m_vram,0x20,RM380Z_SCREENSIZE);
		memset(m_vramchars,0x20,RM380Z_SCREENSIZE);
		memset(m_vramattribs,0x00,RM380Z_SCREENSIZE);
		maxrow=0;
		m_pageAdder=0;
	}
}

// char attribute bits in COS 4.0

// 0=alternate charset
// 1=underline
// 2=dim
// 3=reverse


void rm380z_state::decode_videoram_char(int pos,UINT8& chr,UINT8& attrib)
{
	UINT8 ch1=m_vramchars[pos];
	UINT8 ch2=m_vramattribs[pos];
	
	// "special" (unknown) cases first
	if ((ch1==0x80)&&(ch2==0x04))
	{
		// blank out
		chr=0x20;
		attrib=0;
		return;
	}
	else if ((ch1==0)&&(ch2==8))
	{
		// cursor
		chr=0x20;
		attrib=8;
		return;
	}
	else if ((ch1==0)&&(ch2==0))
	{
		// delete char (?)
		chr=0x20;
		attrib=0;
		return;
	}
	else if ((ch1==4)&&(ch2==4))
	{
		// reversed cursor?
		chr=0x20;
		attrib=0;
		return;
	}
	else if ((ch1==4)&&(ch2==8))
	{
		// normal cursor
		chr=0x20;
		attrib=8;
		return;
	}
	else
	{
		chr=ch1;
		attrib=ch2;

		//printf("unhandled character combination [%x][%x]\n",ch1,ch2);
	}
}

void rm380z_state::scroll_videoram()
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	int lineWidth=0x80;
	if (m_videomode==RM380Z_VIDEOMODE_40COL)
	{
		lineWidth=0x40;
	}

	// scroll up one row of videoram

	for (int row=1;row<RM380Z_SCREENROWS;row++)
	{
		for (int c=0;c<lineWidth;c++)
		{
			int sourceaddr=(row*lineWidth)+c;
			int destaddr=((row-1)*lineWidth)+c;
			
			state->m_vram[destaddr]=state->m_vram[sourceaddr];
			state->m_vramchars[destaddr]=state->m_vramchars[sourceaddr];
			state->m_vramattribs[destaddr]=state->m_vramattribs[sourceaddr];
		}
	}
	
	// the last line is filled with spaces

	for (int c=0;c<lineWidth;c++)
	{
		state->m_vram[((RM380Z_SCREENROWS-1)*lineWidth)+c]=0x20;
		state->m_vramchars[((RM380Z_SCREENROWS-1)*lineWidth)+c]=0x20;
		state->m_vramattribs[((RM380Z_SCREENROWS-1)*lineWidth)+c]=0x00;
	}	
}

// after ctrl-L (clear screen?): routine at EBBD is executed
// EB30??? next line?
// memory at FF02 seems to hold the line counter (same as FBFD)
//
// basics:
// 20e2: prints "Ready:"
// 0195: prints "\n"

WRITE8_MEMBER( rm380z_state::videoram_write )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	//printf("vramw [%x][%x] port0 [%x] fbfd [%x] fbfe [%x] PC [%x]\n",offset,data,state->m_port0,m_fbfd,m_fbfe,cpu_get_pc(machine().device("maincpu")));
	//printf("maxrow is [%x]\n",maxrow);

	int lineWidth=0x80;
	if (m_videomode==RM380Z_VIDEOMODE_40COL)
	{
		lineWidth=0x40;
	}

	if (m_old_fbfd>m_fbfd)
	{
		//printf("WARN: fbfd wrapped\n");
		m_pageAdder+=(0x18*lineWidth);
	}
	m_old_fbfd=m_fbfd;

	//
	 
	int rowadder=(m_fbfe&0x0f)*2;
	int fbfdadder=m_fbfd&0x1f;

	int lineAdder=(rowadder+fbfdadder)*lineWidth;
	int realA=offset+lineAdder+m_pageAdder;

	if ((realA/lineWidth)>maxrow)
	{
		maxrow=realA/lineWidth;
		//printf("setting new maxrow [%x]\n",maxrow);
		if (maxrow>0x17) 
		{
			//printf("ok, scrolling videoram\n");
			scroll_videoram();
		}
	}
	
	int scrollsub=0;
	if (maxrow>0x17)
	{
		scrollsub=(maxrow-0x17)*lineWidth;
	}
	realA-=scrollsub;

	// we suppose videoram is being written as character/attribute couple
	// fbfc 6th bit set=attribute, unset=char

	if (!(state->m_port0&0x40))
	{
		m_vramchars[realA%RM380Z_SCREENSIZE]=data;
	}
	else
	{
		m_vramattribs[realA%RM380Z_SCREENSIZE]=data;
	}

	//

	state->m_mainVideoram[offset]=data;
}

READ8_MEMBER( rm380z_state::videoram_read )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();
	//printf("read from videoram at [%x]\n",offset);
	//return state->m_vram[offset];
	return state->m_mainVideoram[offset];
}

void rm380z_state::putChar(int charnum,int attribs,int x,int y,UINT16* pscr,unsigned char* chsb,int vmode)
{
//	bool attrDim=false;
	bool attrRev=false;
	bool attrUnder=false;
	
	if (attribs&0x02) attrUnder=true;
//	if (attribs&0x04) attrDim=true;
	if (attribs&0x08) attrRev=true;
	
	if ((charnum>0)&&(charnum<=0x7f))
	{
		if (vmode==RM380Z_VIDEOMODE_80COL)
		{
			int basex=RM380Z_CHDIMX*(charnum/RM380Z_NCY);
			int basey=RM380Z_CHDIMY*(charnum%RM380Z_NCY);
			
			int inix=x*(RM380Z_CHDIMX+1);
			int iniy=y*(RM380Z_CHDIMX+1)*RM380Z_SCREENCOLS*RM380Z_CHDIMY;
			
			for (int r=0;r<RM380Z_CHDIMY;r++)
			{
				for (int c=0;c<RM380Z_CHDIMX;c++)
				{
					UINT8 chval=(chsb[((basey+r)*(RM380Z_CHDIMX*RM380Z_NCX))+(basex+c)])==0xff?0:1;
					
					if (attrRev)
					{
						if (chval==0) chval=1;
						else chval=0;
					}
					
					if (attrUnder) 
					{
						if (r==(RM380Z_CHDIMY-1))
						{
							if (attrRev) chval=0;
							else chval=1;
						}
					}
					
					pscr[(inix+c)+(iniy+(r*(RM380Z_CHDIMX+1)*RM380Z_SCREENCOLS))]=chval;
				}
			}
			
			// last pixel of underline
			if (attrUnder) 
			{
				pscr[(inix+RM380Z_CHDIMX)+(iniy+((RM380Z_CHDIMY-1)*(RM380Z_CHDIMX+1)*RM380Z_SCREENCOLS))]=attrRev?0:1;
			}
			
			// if reversed, print another column of pixels on the right
			if (attrRev)
			{
				for (int r=0;r<RM380Z_CHDIMY;r++)
				{
					pscr[(inix+RM380Z_CHDIMX)+(iniy+(r*(RM380Z_CHDIMX+1)*RM380Z_SCREENCOLS))]=1;
				}
			}
		}
		else if (vmode==RM380Z_VIDEOMODE_40COL)
		{
			int basex=RM380Z_CHDIMX*(charnum/RM380Z_NCY);
			int basey=RM380Z_CHDIMY*(charnum%RM380Z_NCY);
			
			int inix=(x*(RM380Z_CHDIMX+1)*2);
			int iniy=y*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)*RM380Z_CHDIMY;
			
			for (int r=0;r<RM380Z_CHDIMY;r++)
			{
				for (int c=0;c<(RM380Z_CHDIMX*2);c+=2)
				{
					UINT8 chval=(chsb[((basey+r)*(RM380Z_CHDIMX*RM380Z_NCX))+(basex+(c/2))])==0xff?0:1;
						
					if (attrRev)
					{
						if (chval==0) chval=1;
						else chval=0;
					}

					if (attrUnder) 
					{
						if (r==(RM380Z_CHDIMY-1))
						{
							if (attrRev) chval=0;
							else chval=1;
						}
					}

					pscr[(inix+c)+(iniy+(r*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=chval;
					pscr[(inix+c+1)+(iniy+(r*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=chval;
				}
			}

			// last 2 pixels of underline
			if (attrUnder) 
			{
				pscr[(inix+(RM380Z_CHDIMX*2))+(iniy+((RM380Z_CHDIMY-1)*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=attrRev?0:1;
				pscr[(inix+(RM380Z_CHDIMX*2)+1)+(iniy+((RM380Z_CHDIMY-1)*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=attrRev?0:1;
			}
		
			// if reversed, print another 2 columns of pixels on the right
			if (attrRev)
			{
				for (int r=0;r<RM380Z_CHDIMY;r++)
				{
					pscr[(inix+(RM380Z_CHDIMX*2))+(iniy+(r*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=1;
					pscr[(inix+(RM380Z_CHDIMX*2)+1)+(iniy+(r*(RM380Z_CHDIMX+1)*(RM380Z_SCREENCOLS)))]=1;
				}
			}
		}
	}
}

void rm380z_state::update_screen(bitmap_t &bitmap)
{
	unsigned char* pChar=machine().region("gfx")->base();
	UINT16* scrptr = &bitmap.pix16(0);
	
	memset(scrptr,0,(RM380Z_SCREENCOLS*(RM380Z_CHDIMX+1))*(RM380Z_SCREENROWS*RM380Z_CHDIMY)*2);

	int lineWidth=0x80;
	int ncols=80;
	
	if (m_videomode==RM380Z_VIDEOMODE_40COL)
	{
		lineWidth=0x40;
		ncols=40;
	}

	for (int row=0;row<RM380Z_SCREENROWS;row++)
	{
		for (int col=0;col<ncols;col++)
		{
			UINT8 curch,attribs;
			decode_videoram_char((row*lineWidth)+col,curch,attribs);
			putChar(curch,attribs,col,row,scrptr,pChar,m_videomode);			

			//putChar(0x4f,10,10,scrptr,pChar,RM380Z_VIDEOMODE_40COL);			
		}
	}
}
