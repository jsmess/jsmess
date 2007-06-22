
#pragma optimize (on)
#pragma debug (off)

uniform sampler2D color_texture;
uniform sampler2D colortable_texture;
uniform vec2      colortable_sz;         // orig size for full bgr
uniform vec2      colortable_pow2_sz;    // orig size for full bgr

#define KERNEL_SIZE 9
const int MaxKernelSize = KERNEL_SIZE;

uniform vec2 Offset[MaxKernelSize];

uniform vec4 KernelValue[MaxKernelSize];

vec4 lutTex2D(in vec2 texcoord)
{
	vec4 color_tex;
	vec2 color_map_coord;
	vec4 color0;
	float colortable_scale = (colortable_sz.x/3.0) / colortable_pow2_sz.x;

	// normalized texture coordinates ..
	color_tex         = texture2D(color_texture, texcoord) * ((colortable_sz.x/3.0)-1.0)/colortable_pow2_sz.x;// lookup space 

	color_map_coord.x = color_tex.b;
	color0.b          = texture2D(colortable_texture, color_map_coord).b;

	color_map_coord.x = color_tex.g + colortable_scale;
	color0.g          = texture2D(colortable_texture, color_map_coord).g;

	color_map_coord.x = color_tex.r + 2.0 * colortable_scale;
	color0.r          = texture2D(colortable_texture, color_map_coord).r;

	return color0;
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

