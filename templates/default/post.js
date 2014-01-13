
//SDL 1.3+
function _SDL_GL_LoadLibrary() {}

// MESS-JavaScript function mappings
var JSMESS = JSMESS || {};
JSMESS.ui_set_show_fps = Module.cwrap('_Z15ui_set_show_fpsi', '', ['number']);
JSMESS.ui_get_show_fps = Module.cwrap('_Z15ui_get_show_fpsv', 'number');
