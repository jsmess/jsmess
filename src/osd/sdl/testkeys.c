//============================================================
//
//  testkey.c - A small utility to analyze SDL keycodes
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//  testkeys by couriersud
//
//============================================================

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <SDL/SDL.h>

typedef struct _key_lookup_table key_lookup_table;

struct _key_lookup_table 
{
	int code;
	const char *name;
};

#define KE(x) { x, #x }

static key_lookup_table sdl_lookup[] =
{
	KE(SDLK_UNKNOWN),
	KE(SDLK_FIRST),
	KE(SDLK_BACKSPACE),
	KE(SDLK_TAB),
	KE(SDLK_CLEAR),
	KE(SDLK_RETURN),
	KE(SDLK_PAUSE),
	KE(SDLK_ESCAPE),
	KE(SDLK_SPACE),
	KE(SDLK_EXCLAIM),
	KE(SDLK_QUOTEDBL),
	KE(SDLK_HASH),
	KE(SDLK_DOLLAR),
	KE(SDLK_AMPERSAND),
	KE(SDLK_QUOTE),
	KE(SDLK_LEFTPAREN),
	KE(SDLK_RIGHTPAREN),
	KE(SDLK_ASTERISK),
	KE(SDLK_PLUS),
	KE(SDLK_COMMA),
	KE(SDLK_MINUS),
	KE(SDLK_PERIOD),
	KE(SDLK_SLASH),
	KE(SDLK_0),
	KE(SDLK_1),
	KE(SDLK_2),
	KE(SDLK_3),
	KE(SDLK_4),
	KE(SDLK_5),
	KE(SDLK_6),
	KE(SDLK_7),
	KE(SDLK_8),
	KE(SDLK_9),
	KE(SDLK_COLON),
	KE(SDLK_SEMICOLON),
	KE(SDLK_LESS),
	KE(SDLK_EQUALS),
	KE(SDLK_GREATER),
	KE(SDLK_QUESTION),
	KE(SDLK_AT),
	KE(SDLK_LEFTBRACKET),
	KE(SDLK_BACKSLASH),
	KE(SDLK_RIGHTBRACKET),
	KE(SDLK_CARET),
	KE(SDLK_UNDERSCORE),
	KE(SDLK_BACKQUOTE),
	KE(SDLK_a),
	KE(SDLK_b),
	KE(SDLK_c),
	KE(SDLK_d),
	KE(SDLK_e),
	KE(SDLK_f),
	KE(SDLK_g),
	KE(SDLK_h),
	KE(SDLK_i),
	KE(SDLK_j),
	KE(SDLK_k),
	KE(SDLK_l),
	KE(SDLK_m),
	KE(SDLK_n),
	KE(SDLK_o),
	KE(SDLK_p),
	KE(SDLK_q),
	KE(SDLK_r),
	KE(SDLK_s),
	KE(SDLK_t),
	KE(SDLK_u),
	KE(SDLK_v),
	KE(SDLK_w),
	KE(SDLK_x),
	KE(SDLK_y),
	KE(SDLK_z),
	KE(SDLK_DELETE),
	KE(SDLK_WORLD_0),
	KE(SDLK_WORLD_1),
	KE(SDLK_WORLD_2),
	KE(SDLK_WORLD_3),
	KE(SDLK_WORLD_4),
	KE(SDLK_WORLD_5),
	KE(SDLK_WORLD_6),
	KE(SDLK_WORLD_7),
	KE(SDLK_WORLD_8),
	KE(SDLK_WORLD_9),
	KE(SDLK_WORLD_10),
	KE(SDLK_WORLD_11),
	KE(SDLK_WORLD_12),
	KE(SDLK_WORLD_13),
	KE(SDLK_WORLD_14),
	KE(SDLK_WORLD_15),
	KE(SDLK_WORLD_16),
	KE(SDLK_WORLD_17),
	KE(SDLK_WORLD_18),
	KE(SDLK_WORLD_19),
	KE(SDLK_WORLD_20),
	KE(SDLK_WORLD_21),
	KE(SDLK_WORLD_22),
	KE(SDLK_WORLD_23),
	KE(SDLK_WORLD_24),
	KE(SDLK_WORLD_25),
	KE(SDLK_WORLD_26),
	KE(SDLK_WORLD_27),
	KE(SDLK_WORLD_28),
	KE(SDLK_WORLD_29),
	KE(SDLK_WORLD_30),
	KE(SDLK_WORLD_31),
	KE(SDLK_WORLD_32),
	KE(SDLK_WORLD_33),
	KE(SDLK_WORLD_34),
	KE(SDLK_WORLD_35),
	KE(SDLK_WORLD_36),
	KE(SDLK_WORLD_37),
	KE(SDLK_WORLD_38),
	KE(SDLK_WORLD_39),
	KE(SDLK_WORLD_40),
	KE(SDLK_WORLD_41),
	KE(SDLK_WORLD_42),
	KE(SDLK_WORLD_43),
	KE(SDLK_WORLD_44),
	KE(SDLK_WORLD_45),
	KE(SDLK_WORLD_46),
	KE(SDLK_WORLD_47),
	KE(SDLK_WORLD_48),
	KE(SDLK_WORLD_49),
	KE(SDLK_WORLD_50),
	KE(SDLK_WORLD_51),
	KE(SDLK_WORLD_52),
	KE(SDLK_WORLD_53),
	KE(SDLK_WORLD_54),
	KE(SDLK_WORLD_55),
	KE(SDLK_WORLD_56),
	KE(SDLK_WORLD_57),
	KE(SDLK_WORLD_58),
	KE(SDLK_WORLD_59),
	KE(SDLK_WORLD_60),
	KE(SDLK_WORLD_61),
	KE(SDLK_WORLD_62),
	KE(SDLK_WORLD_63),
	KE(SDLK_WORLD_64),
	KE(SDLK_WORLD_65),
	KE(SDLK_WORLD_66),
	KE(SDLK_WORLD_67),
	KE(SDLK_WORLD_68),
	KE(SDLK_WORLD_69),
	KE(SDLK_WORLD_70),
	KE(SDLK_WORLD_71),
	KE(SDLK_WORLD_72),
	KE(SDLK_WORLD_73),
	KE(SDLK_WORLD_74),
	KE(SDLK_WORLD_75),
	KE(SDLK_WORLD_76),
	KE(SDLK_WORLD_77),
	KE(SDLK_WORLD_78),
	KE(SDLK_WORLD_79),
	KE(SDLK_WORLD_80),
	KE(SDLK_WORLD_81),
	KE(SDLK_WORLD_82),
	KE(SDLK_WORLD_83),
	KE(SDLK_WORLD_84),
	KE(SDLK_WORLD_85),
	KE(SDLK_WORLD_86),
	KE(SDLK_WORLD_87),
	KE(SDLK_WORLD_88),
	KE(SDLK_WORLD_89),
	KE(SDLK_WORLD_90),
	KE(SDLK_WORLD_91),
	KE(SDLK_WORLD_92),
	KE(SDLK_WORLD_93),
	KE(SDLK_WORLD_94),
	KE(SDLK_WORLD_95),
	KE(SDLK_KP0),
	KE(SDLK_KP1),
	KE(SDLK_KP2),
	KE(SDLK_KP3),
	KE(SDLK_KP4),
	KE(SDLK_KP5),
	KE(SDLK_KP6),
	KE(SDLK_KP7),
	KE(SDLK_KP8),
	KE(SDLK_KP9),
	KE(SDLK_KP_PERIOD),
	KE(SDLK_KP_DIVIDE),
	KE(SDLK_KP_MULTIPLY),
	KE(SDLK_KP_MINUS),
	KE(SDLK_KP_PLUS),
	KE(SDLK_KP_ENTER),
	KE(SDLK_KP_EQUALS),
	KE(SDLK_UP),
	KE(SDLK_DOWN),
	KE(SDLK_RIGHT),
	KE(SDLK_LEFT),
	KE(SDLK_INSERT),
	KE(SDLK_HOME),
	KE(SDLK_END),
	KE(SDLK_PAGEUP),
	KE(SDLK_PAGEDOWN),
	KE(SDLK_F1),
	KE(SDLK_F2),
	KE(SDLK_F3),
	KE(SDLK_F4),
	KE(SDLK_F5),
	KE(SDLK_F6),
	KE(SDLK_F7),
	KE(SDLK_F8),
	KE(SDLK_F9),
	KE(SDLK_F10),
	KE(SDLK_F11),
	KE(SDLK_F12),
	KE(SDLK_F13),
	KE(SDLK_F14),
	KE(SDLK_F15),
	KE(SDLK_NUMLOCK),
	KE(SDLK_CAPSLOCK),
	KE(SDLK_SCROLLOCK),
	KE(SDLK_RSHIFT),
	KE(SDLK_LSHIFT),
	KE(SDLK_RCTRL),
	KE(SDLK_LCTRL),
	KE(SDLK_RALT),
	KE(SDLK_LALT),
	KE(SDLK_RMETA),
	KE(SDLK_LMETA),
	KE(SDLK_LSUPER),
	KE(SDLK_RSUPER),
	KE(SDLK_MODE),
	KE(SDLK_COMPOSE),
	KE(SDLK_HELP),
	KE(SDLK_PRINT),
	KE(SDLK_SYSREQ),
	KE(SDLK_BREAK),
	KE(SDLK_MENU),
	KE(SDLK_POWER),
	KE(SDLK_EURO),
	KE(SDLK_UNDO),
	KE(SDLK_LAST),
	KE(-1)
};

static const char * lookup_key_name(const key_lookup_table *kt, int kc)
{
	int i=0;
	while (kt[i].code>=0)
	{
		if (kc==kt[i].code)
			return kt[i].name;
		i++;
	}
	return NULL;
}

char *UNICODE_to_UTF8(char *utf8, int utf8_len, const wchar_t *unicode, int uni_len)
{
    int i, j;

    for (i = 0, j = 0; i < uni_len && j < utf8_len; ++i, ++j)
    {
    if (unicode[i] < 0x80)
    {
        utf8[j] = unicode[i] & 0x7F;
    }
    else if (unicode[i] < 0x800 && j+1 < utf8_len)
    {
        utf8[j] = 0xC0 | (unicode[i] >> 6);
        utf8[++j] = 0x80 | (unicode[i] & 0x3F);
    }
    else if (unicode[i] < 0x10000 && j+2 < utf8_len)
    {
        utf8[j] = 0xE0 | (unicode[i] >> 12);
        utf8[++j] = 0x80 | ((unicode[i] >> 6) & 0x3F);
        utf8[++j] = 0x80 | (unicode[i] & 0x3F);
    }
    else if (unicode[i] < 0x200000 && j+3 < utf8_len)
    {
        utf8[j] = 0xF0 | (unicode[i] >> 18);
        utf8[++j] = 0x80 | ((unicode[i] >> 12) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 6) & 0x3F);
        utf8[++j] = 0x80 | (unicode[i] & 0x3F);
    }
    else if (unicode[i] < 0x4000000 && j+4 < utf8_len)
    {
        utf8[j] = 0xF8 | (unicode[i] >> 24);
        utf8[++j] = 0x80 | ((unicode[i] >> 18) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 12) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 6) & 0x3F);
        utf8[++j] = 0x80 | (unicode[i] & 0x3F);
    }
    else if (unicode[i] < 0x80000000 && j+5 < utf8_len)
    {
            utf8[j] = 0xFC | (unicode[i] >> 30);
        utf8[++j] = 0x80 | ((unicode[i] >> 24) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 18) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 12) & 0x3F);
        utf8[++j] = 0x80 | ((unicode[i] >> 6) & 0x3F);
        utf8[++j] = 0x80 | (unicode[i] & 0x3F);
    }
    }

    utf8[j] = 0;

    return utf8;
}

#ifdef SDLMAME_WIN32
int utf8_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	SDL_Event event;
	int quit = 0;
	char buf[20];
	wchar_t uc;

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
							SDL_GetError());
		exit(1);
	}
	SDL_SetVideoMode(100, 50, 16, SDL_ANYFORMAT);
	SDL_EnableUNICODE(1);
	while(SDL_PollEvent(&event) || !quit) {
		switch(event.type) {
		case SDL_QUIT:
			quit = 1;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				quit=1;
			else
			{
				memset(buf, 0, 19);
				uc = event.key.keysym.unicode;
				UNICODE_to_UTF8(buf, 2, &uc, 1);
				printf("KEYCODE_XY %s 0x%x 0x%x %s\n",
					lookup_key_name(sdl_lookup, event.key.keysym.sym),
					(int) event.key.keysym.scancode, 
					(int) event.key.keysym.unicode, 
					buf);
			}
			break;
		}
		event.type = 0;

		#ifdef SDLMAME_OS2
		SDL_Delay( 10 );
		#endif
	}
	SDL_Quit();
	return(0);
}
