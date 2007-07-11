
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform sampler2D colortable_texture;
uniform vec2      colortable_sz;      // ct size
uniform vec2      colortable_pow2_sz; // pow2 ct size

#define KERNEL_SIZE 9
const int MaxKernelSize = KERNEL_SIZE;

uniform vec2 Offset[MaxKernelSize];

uniform vec4 KernelValue[MaxKernelSize];

vec4 lutTex2D(in vec2 texcoord)
{
	vec4 color_tex;
	vec2 color_map_coord;

	// normalized texture coordinates ..
	color_tex = texture2D(color_texture, texcoord);

	// GL_UNSIGNED_SHORT GL_ALPHA in ALPHA16 conversion:
	// general: f = c / ((2*N)-1), c color bitfield, N number of bits
	// ushort:  c = ((2**16)-1)*f;
	color_map_coord.x  = 65536.0 * color_tex.a;

	// map it to the 2D lut table
	color_map_coord.y = floor(color_map_coord.x/colortable_sz.x);
	color_map_coord.x =   mod(color_map_coord.x,colortable_sz.x);

	return texture2D(colortable_texture, color_map_coord/colortable_pow2_sz);
}

void main()
{
	vec4 sum = vec4(0.0);
	int i;

	// sum it up ..
	for(i=0; i<KERNEL_SIZE; i++)
	{
		sum += lutTex2D(gl_TexCoord[0].st + Offset[i]) * KernelValue[i];
	}
	
	gl_FragColor = sum;
}

