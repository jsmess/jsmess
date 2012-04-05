function _SDL_Linked_Version() {}
function _SDL_GetWMInfo() {}
function _SDL_VideoDriverName() {}
function _SDL_ThreadID() {}
function _SDL_AudioDriverName() {}

function __Z20jsmess_set_main_loopR16device_scheduler($this) {
  jsmess_main_loop($this);
  _abort(0);
}

function jsmess_main_loop($this) {
  __ZN16device_scheduler9timesliceEv($this);
  setTimeout(function() { jsmess_main_loop($this) }, 1);
}
