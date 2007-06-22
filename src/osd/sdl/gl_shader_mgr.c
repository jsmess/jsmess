
#include "gl_shader_mgr.h"

#ifdef GLSL_SOURCE_ON_DISK

static const char * glsl_fsh_files [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER] =
{
 {"/tmp/glsl_idx16_lut.fsh",								// idx16 lut plain
  "/tmp/glsl_idx16_lut_bilinear.fsh", "/tmp/glsl_idx16_lut_cgauss.fsh" },		// idx16 lut bilinear, gaussian

 {"/tmp/glsl_rgb32_lut.fsh",								// rgb32 lut plain
  "/tmp/glsl_rgb32_lut_bilinear.fsh", "/tmp/glsl_rgb32_lut_cgauss.fsh" },		// rgb32 lut gaussian

 {"/tmp/glsl_rgb32_dir.fsh",								// rgb32 dir plain
  "/tmp/glsl_rgb32_dir_bilinear.fsh", "/tmp/glsl_rgb32_dir_cgauss.fsh" },		// rgb32 dir gaussian
};

#else // GLSL_SOURCE_ON_DISK

#include "shader/glsl_general.vsh.c"

#include "shader/glsl_idx16_lut.fsh.c"
#include "shader/glsl_idx16_lut_bilinear.fsh.c"
#include "shader/glsl_idx16_lut_cgauss.fsh.c"

#include "shader/glsl_rgb32_lut.fsh.c"
#include "shader/glsl_rgb32_lut_bilinear.fsh.c"
#include "shader/glsl_rgb32_lut_cgauss.fsh.c"

#include "shader/glsl_rgb32_dir.fsh.c"
#include "shader/glsl_rgb32_dir_bilinear.fsh.c"
#include "shader/glsl_rgb32_dir_cgauss.fsh.c"

static const char * glsl_fsh_sources [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER] =
{
 {glsl_idx16_lut_fsh_src,								// idx16 lut plain
  glsl_idx16_lut_bilinear_fsh_src, glsl_idx16_lut_cgauss_fsh_src },			// idx16 lut bilinear, gaussian

 {glsl_rgb32_lut_fsh_src,								// rgb32 lut plain
  glsl_rgb32_lut_bilinear_fsh_src, glsl_rgb32_lut_cgauss_fsh_src },			// rgb32 lut bilinear, gaussian

 {glsl_rgb32_dir_fsh_src,								// rgb32 dir plain
  glsl_rgb32_dir_bilinear_fsh_src, glsl_rgb32_dir_cgauss_fsh_src },			// rgb32 dir bilinear, gaussian
};

#endif // GLSL_SOURCE_ON_DISK

GLhandleARB glsl_programs [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, gaussian */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, gaussian */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, gaussian */ 
};

GLhandleARB glsl_general_vsh = 0; // one for all

GLhandleARB glsl_fsh_shader [GLSL_SHADER_TYPE_NUMBER][GLSL_SHADER_FEAT_NUMBER] =
{
 {  0, 0, 0 },  /* idx16 lut: plain, bilinear, gaussian */ 
 {  0, 0, 0 },  /* rgb32 lut: plain, bilinear, gaussian */ 
 {  0, 0, 0 },  /* rgb32 dir: plain, bilinear, gaussian */ 
};

int glsl_shader_init(sdl_info *sdl)
{
	int i,j, err;

        err = gl_shader_loadExtention((PFNGLGETPROCADDRESSOS)SDL_GL_GetProcAddress);
	if(err) return err;
	
	#ifdef GLSL_SOURCE_ON_DISK
	err = gl_compile_shader_file  ( &glsl_general_vsh, GL_VERTEX_SHADER_ARB, "/tmp/glsl_general.vsh", 0 /* verbose */ );
	#else
	err = gl_compile_shader_source( &glsl_general_vsh, GL_VERTEX_SHADER_ARB, glsl_general_vsh_src, 0 /* verbose */ );
	#endif

	if(err) return err;

	for (i=0; !err && i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; !err && j<GLSL_SHADER_FEAT_NUMBER; j++)
		{
		#ifdef GLSL_SOURCE_ON_DISK
			if(glsl_fsh_files[i][j])
				err = gl_compile_shader_files  (&glsl_programs[i][j], &glsl_general_vsh, &glsl_fsh_shader[i][j],
								NULL /*precompiled*/, glsl_fsh_files[i][j], 0);
		#else
			if(glsl_fsh_sources[i][j])
				err = gl_compile_shader_sources(&glsl_programs[i][j], &glsl_general_vsh, &glsl_fsh_shader[i][j],
								NULL /*precompiled*/, glsl_fsh_sources[i][j]);
		#endif
		}
	}
	return err;
}

int glsl_shader_free(sdl_info *sdl)
{
	int i,j;

	pfn_glUseProgramObjectARB(0); // back to fixed function pipeline
        glFinish();

	(void) gl_delete_shader( NULL,  &glsl_general_vsh, NULL); 

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_NUMBER; j++)
		{
			(void) gl_delete_shader( NULL, NULL, &glsl_fsh_shader[i][j]);
		}
	}

	for (i=0; i<GLSL_SHADER_TYPE_NUMBER; i++)
	{
		for (j=0; j<GLSL_SHADER_FEAT_NUMBER; j++)
		{
			(void) gl_delete_shader( &glsl_programs[i][j], NULL, NULL);
		}
	}

	return 0;
}

