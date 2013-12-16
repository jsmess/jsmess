function _SDL_Linked_Version() {}
function _SDL_GetWMInfo() {}
function _SDL_VideoDriverName() {}
function _SDL_ThreadID() {}
function _SDL_AudioDriverName() {}

//SDL 1.3+
function _SDL_GetNumVideoDrivers() {}
function _SDL_GetCurrentVideoDriver() {}
function _SDL_GetNumVideoDisplays() {}
function _SDL_GetNumAudioDrivers() {}
function _SDL_SetWindowDisplayMode() {}
function _SDL_CreateWindow() {}
function _SDL_ShowWindow() {}
function _SDL_SetWindowFullscreen() {}
function _SDL_GetWindowSize() {}
function _SDL_RaiseWindow() {}
function _SDL_SetWindowGrab() {}
function _SDL_CreateRenderer() {}
function _SDL_GetRendererInfo() {}
function _SDL_GetCurrentDisplayMode() {}
function _SDL_CreateTexture() {}
function _SDL_GetCurrentAudioDriver() {}
function _SDL_EventState() {}
function _SDL_GetWindowGrab() {}
function _SDL_SetCursor() {}
function _SDL_GetDesktopDisplayMode() {}
function _SDL_PumpEvents() {}
function _SDL_QueryTexture() {}
function _SDL_PixelFormatEnumToMasks() {}
function _SDL_LockTexture() {}
function _SDL_UnlockTexture() {}
function _SDL_RenderCopy() {}
function _SDL_RenderPresent() {}
function _SDL_GL_LoadLibrary() {}
function _SDL_GL_DeleteContext() {}
function _SDL_GL_DestroyWindow() {}

// MESS-JavaScript function mappings
var JSMESS = JSMESS || {};
JSMESS.ui_set_show_fps = Module.cwrap('_Z15ui_set_show_fpsi', '', ['number']);
JSMESS.ui_get_show_fps = Module.cwrap('_Z15ui_get_show_fpsv', 'number');
