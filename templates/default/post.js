
//SDL 1.3+
function _SDL_GL_LoadLibrary() {}

// MESS-JavaScript function mappings
var JSMESS = JSMESS || {};
JSMESS.get_ui = Module.cwrap('_Z9js_get_uiv', 'number');
JSMESS.ui_set_show_fps = Module.cwrap('_ZN10ui_manager12set_show_fpsEb', '', ['number', 'number']);
JSMESS.ui_get_show_fps = Module.cwrap('_ZNK10ui_manager8show_fpsEv', 'number', ['number']);
