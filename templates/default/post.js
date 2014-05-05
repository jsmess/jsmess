
//SDL 1.3+
function _SDL_GL_LoadLibrary() {}

// MESS-JavaScript function mappings
var JSMESS = JSMESS || {};
JSMESS.get_machine = Module.cwrap('_Z14js_get_machinev', 'number');
JSMESS.get_ui = Module.cwrap('_Z9js_get_uiv', 'number');
JSMESS.get_sound = Module.cwrap('_Z12js_get_soundv', 'number');
JSMESS.ui_set_show_fps = Module.cwrap('_ZN10ui_manager12set_show_fpsEb', '', ['number', 'number']);
JSMESS.ui_get_show_fps = Module.cwrap('_ZNK10ui_manager8show_fpsEv', 'number', ['number']);
JSMESS.sound_manager_mute = Module.cwrap('_ZN13sound_manager4muteEbh', '', ['number', 'number', 'number']);
JSMESS.sdl_pauseaudio = Module.cwrap('SDL_PauseAudio', '', ['number']);
