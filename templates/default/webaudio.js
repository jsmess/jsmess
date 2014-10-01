// jsmess web audio backend v0.2
// katelyn gadd - kg at luminance dot org ; @antumbral on twitter

var jsmess_web_audio = (function () {

var context = null;
var gain_node = null;
var buffer_insert_point = null;
var pending_buffers = [];

var numChannels = 2; // constant in jsmess
var sampleScale = 32766;
var prebufferDuration = 100 / 1000;

function lazy_init () {
  if (context || typeof AudioContext == 'undefined')
    return;

  context = new AudioContext();

  gain_node = context.createGain();
  gain_node.gain.value = 1.0;
  gain_node.connect(context.destination);
};

function set_mastervolume (
  // even though it's 'attenuation' the value is negative, so...
  attenuation_in_decibels
) {
  lazy_init();
  if (!context) return;

  // http://stackoverflow.com/questions/22604500/web-audio-api-working-with-decibels
  // seemingly incorrect/broken. figures. welcome to Web Audio
  // var gain_web_audio = 1.0 - Math.pow(10, 10 / attenuation_in_decibels);

  // HACK: Max attenuation in JSMESS appears to be 32.
  // Hit ' then left/right arrow to test.
  // FIXME: This is linear instead of log10 scale.
  var gain_web_audio = 1.0 + (+attenuation_in_decibels / +32);
  if (gain_web_audio < +0)
    gain_web_audio = +0;
  else if (gain_web_audio > +1)
    gain_web_audio = +1;

  gain_node.gain.value = gain_web_audio;
};

function update_audio_stream (
  pBuffer,           // pointer into emscripten heap. int16 samples
  samples_this_frame // int. number of samples at pBuffer address.
) {
  lazy_init();
  if (!context) return;

  var buffer = context.createBuffer(
    numChannels, samples_this_frame, 
    // JSMESS already initializes its mixer to use the context sampling rate.
    context.sampleRate
  );

  for (
    var channel_left  = buffer.getChannelData(0),
        channel_right = buffer.getChannelData(1),
        i = 0,
        l = samples_this_frame | 0;
    i < l;
    i++
  ) {
    var offset = 
      // divide by sizeof(INT16) since pBuffer is offset
      //  in bytes
      ((pBuffer / 2) | 0) +
      ((i * 2) | 0);

    var left_sample = HEAP16[offset];
    var right_sample = HEAP16[(offset + 1) | 0];

    // normalize from signed int16 to signed float
    var left_sample_float = left_sample / sampleScale;
    var right_sample_float = right_sample / sampleScale;

    channel_left[i] = left_sample_float;
    channel_right[i] = right_sample_float;
  }

  pending_buffers.push(buffer);

  tick();
};

function tick () {
  // Note: this is the time the web audio mixer has mixed up to,
  //  not the actual current time.
  var now = context.currentTime;

  // prebuffering
  if (buffer_insert_point === null) {
    var total_buffered_seconds = 0;

    for (var i = 0, l = pending_buffers.length; i < l; i++) {
      var buffer = pending_buffers[i];
      total_buffered_seconds += buffer.duration;
    }

    // Buffer not full enough? abort
    if (total_buffered_seconds < prebufferDuration)
      return;
  }

  // FIXME/TODO: It's possible for us to burn through the whole
  //  chunk of prebuffered audio. At that point it seems like
  //  JSMESS never catches up and our sound glitches forever.

  var insert_point = (buffer_insert_point === null)
    ? now
    : buffer_insert_point;

  if (pending_buffers.length) {
    for (var i = 0, l = pending_buffers.length; i < l; i++) {
      var buffer = pending_buffers[i];

      var source_node = context.createBufferSource();
      source_node.buffer = buffer;
      source_node.connect(gain_node);
      source_node.start(insert_point);

      insert_point += buffer.duration;
    }

    pending_buffers.length = 0;
    buffer_insert_point = insert_point;

    if (buffer_insert_point <= now)
      buffer_insert_point = now;
  }
};
function get_context() {
  return context;
};

return {
  set_mastervolume: set_mastervolume,
  update_audio_stream: update_audio_stream,
  get_context: get_context
};

})();

jsmess_set_mastervolume = jsmess_web_audio.set_mastervolume;
jsmess_update_audio_stream = jsmess_web_audio.update_audio_stream;
