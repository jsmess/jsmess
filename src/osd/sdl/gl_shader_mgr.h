
#ifndef GL_SHADER_MGR_H
#define GL_SHADER_MGR_H

#include <SDL/SDL.h>

// OpenGL headers
#ifdef SDLMAME_MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <SDL/SDL_opengl.h>

#include "gl_shader_tool.h"

// OSD headers
#include "window.h"

// #define GLSL_SOURCE_ON_DISK 1

typedef enum {
	GLSL_SHADER_TYPE_IDX16,
	GLSL_SHADER_TYPE_RGB32_LUT,
	GLSL_SHADER_TYPE_RGB32_DIRECT,
	GLSL_SHADER_TYPE_NUMBER
} GLSL_SHADER_TYPE;

typedef enum {
	GLSL_SHADER_FEAT_PLAIN,
	GLSL_SHADER_FEAT_BILINEAR,
	GLSL_SHADER_FEAT_INT_NUMBER,
	GLSL_SHADER_FEAT_CUSTOM = GLSL_SHADER_FEAT_INT_NUMBER,
	GLSL_SHADER_FEAT_MAX_NUMBER,
} GLSL_SHADER_FEATURE;

/**
 * returns 0 if ok, otherwise an error value
 */
int glsl_shader_init(sdl_info *sdl);

/**
 * returns 0 if ok, otherwise an error value
 */
int glsl_shader_free(sdl_info *sdl);

/**
 * returns the GLSL program if ok/available, otherwise 0
 */
GLhandleARB glsl_shader_get_program(int glslShaderType, int glslShaderFeature);

const char * glsl_shader_get_filter_name(int glslShaderFeature);

int glsl_shader_add_custom(sdl_info *sdl, const char * custShaderPrefix);

#endif // GL_SHADER_MGR_H
