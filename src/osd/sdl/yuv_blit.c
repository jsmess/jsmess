
// YUV Blitters
//
// This should be included at the end of drawsdl.c


#define CU_CLAMP(v, a, b)	((v < a)? a: ((v > b)? b: v))
#define RGB2YUV_F(r,g,b,y,u,v) \
	 (y) = (0.299*(r) + 0.587*(g) + 0.114*(b) ); \
	 (u) = (-0.169*(r) - 0.331*(g) + 0.5*(b) + 128); \
	 (v) = (0.5*(r) - 0.419*(g) - 0.081*(b) + 128); \
	(y) = CU_CLAMP(y,0,255); \
	(u) = CU_CLAMP(u,0,255); \
	(v) = CU_CLAMP(v,0,255)

#define RGB2YUV(r,g,b,y,u,v) \
	 (y) = ((  8453*(r) + 16594*(g) +  3223*(b) +  524288) >> 15); \
	 (u) = (( -4878*(r) -  9578*(g) + 14456*(b) + 4210688) >> 15); \
	 (v) = (( 14456*(r) - 12105*(g) -  2351*(b) + 4210688) >> 15)

#ifdef LSB_FIRST	 
#define Y1MASK  0x000000FF
#define  UMASK  0x0000FF00
#define Y2MASK  0x00FF0000
#define  VMASK  0xFF000000
#define Y1SHIFT  0
#define  USHIFT  8
#define Y2SHIFT 16
#define  VSHIFT 24
#else
#define Y1MASK  0xFF000000
#define  UMASK  0x00FF0000
#define Y2MASK  0x0000FF00
#define  VMASK  0x000000FF
#define Y1SHIFT 24
#define  USHIFT 16
#define Y2SHIFT  8
#define  VSHIFT  0
#endif

#define YMASK  (Y1MASK|Y2MASK)
#define UVMASK (UMASK|VMASK)


static void yuv_lookup_set(sdl_window_info *window, unsigned int pen, unsigned char red, 
			unsigned char green, unsigned char blue)
{
	UINT32 y,u,v;

	RGB2YUV(red,green,blue,y,u,v);

	/* Storing this data in YUYV order simplifies using the data for
	   YUY2, both with and without smoothing... */
	window->yuv_lookup[pen]=(y<<Y1SHIFT)|(u<<USHIFT)|(y<<Y2SHIFT)|(v<<VSHIFT);
}

void yuv_lookup_init(sdl_window_info *window)
{
	unsigned char r,g,b;
	if (window->yuv_lookup == NULL)
		window->yuv_lookup = malloc_or_die(65536*sizeof(UINT32));
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				int idx = (r << 10) | (g << 5) | b;
				yuv_lookup_set(window, idx, 
					(r << 3) | (r >> 2),
					(g << 3) | (g >> 2),
					(b << 3) | (b >> 2));
			}
}

static void yuv_RGB_to_YV12(UINT16 *bitmap, int bw, sdl_window_info *window)
{
	int x, y;
	UINT8 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src; 
	UINT16 *src2;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0];
	int u1,v1,y1,u2,v2,y2,u3,v3,y3,u4,v4,y4;	  /* 12 */
     
	for(y=0;y<window->yuv_ovl_height;y+=2)
	{
		src=bitmap + (y * bw) ;
		src2=src + bw;

		dest_y = window->yuvsurf->pixels[0] + y * window->yuvsurf->pitches[0];
		dest_v = window->yuvsurf->pixels[1] + (y>>1) * window->yuvsurf->pitches[1];
		dest_u = window->yuvsurf->pixels[2] + (y>>1) * window->yuvsurf->pitches[2];

		for(x=0;x<window->yuv_ovl_width;x+=2)
		{
			v1 = lookup[src[x]];
			y1 = (v1>>Y1SHIFT) & 0xff;
			u1 = (v1>>USHIFT)  & 0xff;
			v1 = (v1>>VSHIFT)  & 0xff;

			v2 = lookup[src[x+1]];
			y2 = (v2>>Y1SHIFT) & 0xff;
			u2 = (v2>>USHIFT)  & 0xff;
			v2 = (v2>>VSHIFT)  & 0xff;

			v3 = lookup[src2[x]];
			y3 = (v3>>Y1SHIFT) & 0xff;
			u3 = (v3>>USHIFT)  & 0xff;
			v3 = (v3>>VSHIFT)  & 0xff;

			v4 = lookup[src2[x+1]];
			y4 = (v4>>Y1SHIFT) & 0xff;
			u4 = (v4>>USHIFT)  & 0xff;
			v4 = (v4>>VSHIFT)  & 0xff;

			dest_y[x] = y1;
			dest_y[x+yuv_pitch] = y3;
			dest_y[x+1] = y2;
			dest_y[x+yuv_pitch+1] = y4;

			dest_u[x>>1] = (u1+u2+u3+u4)/4;
			dest_v[x>>1] = (v1+v2+v3+v4)/4;
			
		}
	}
}

static void yuv_RGB_to_YV12X2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int x,y;
	UINT16 *dest_y;
	UINT8 *dest_u;
	UINT8 *dest_v;
	UINT16 *src;
	int yuv_pitch = window->yuvsurf->pitches[0];
	int u1,v1,y1;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;

		dest_y = window->yuvsurf->pixels[0] + 2*y * window->yuvsurf->pitches[0];
		dest_v = window->yuvsurf->pixels[1] + y * window->yuvsurf->pitches[1];
		dest_u = window->yuvsurf->pixels[2] + y * window->yuvsurf->pitches[2];
		for(x=0;x<window->yuv_ovl_width;x++)
		{
			v1 = window->yuv_lookup[src[x]];
			y1 = (v1>>Y1SHIFT) & 0xff;
			u1 = (v1>>USHIFT)  & 0xff;
			v1 = (v1>>VSHIFT)  & 0xff;

			dest_y[x+yuv_pitch/2]=y1<<8|y1;
			dest_y[x]=y1<<8|y1;
			dest_u[x] = u1;
			dest_v[x] = v1;
		}
	}
}

static void yuv_RGB_to_YUY2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 p1,p2,uv;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0]/4;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;
		end=src+window->yuv_ovl_width;

		dest = (UINT32 *) window->yuvsurf->pixels[0];
		dest += y * yuv_pitch;
		for(; src<end; src+=2)
		{
			p1  = lookup[src[0]];
			p2  = lookup[src[1]];
			uv = (p1&UVMASK)>>1;
			uv += (p2&UVMASK)>>1;
			*dest++ = (p1&Y1MASK)|(p2&Y2MASK)|(uv&UVMASK);
		}
	}
}

static void yuv_RGB_to_YUY2X2(UINT16 *bitmap, int bw, sdl_window_info *window)
{		/* this one is used when scale==2 */
	unsigned int y;
	UINT32 *dest;
	UINT16 *src;
	UINT16 *end;
	UINT32 *lookup = window->yuv_lookup;
	int yuv_pitch = window->yuvsurf->pitches[0]/4;

	for(y=0;y<window->yuv_ovl_height;y++)
	{
		src=bitmap + (y * bw) ;
		end=src+window->yuv_ovl_width;

		dest = (UINT32 *) window->yuvsurf->pixels[0];
		dest += (y<<1) * yuv_pitch;
		for(; src<end; src++)
		{
			dest[0] = dest[yuv_pitch]  = lookup[src[0]];
			dest++;
		}
	}
}

