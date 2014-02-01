var JSMESS = JSMESS || {};
JSMESS._readySet = false;
JSMESS._readyList = [];
JSMESS._runReadies = function() {
	if (JSMESS._readyList) {
		for (var r=0; r < JSMESS._readyList.length; r++) {
			JSMESS._readyList[r].call(window, []);
		};
		JSMESS._readyList = [];
	};
};
JSMESS._readyCheck = function() {
	if (JSMESS.running) {
		JSMESS._runReadies();
	} else {
		JSMESS._readySet = setTimeout(JSMESS._readyCheck, 10);
	};
};
JSMESS.ready = function(r) {
	if (JSMESS.running) {
		r.call(window, []);
	} else {
		JSMESS._readyList.push(function() { return r.call(window, []); } );
		if (!(JSMESS._readySet)) JSMESS._readyCheck();
	};
};

var gamename = 'GAME_FILE';
var game_file = null;
var bios_filenames = 'BIOS_FILES'.split(' ');
var bios_files = {};
var file_countdown = 0;
if (bios_filenames.length !== 0 && bios_filenames[0] !== '') {
  file_countdown += bios_filenames.length;
}
if (gamename !== '') {
  file_countdown++;
}

var newCanvas = document.createElement('canvas');
newCanvas.id = 'canvas';
newCanvas.width = 256;
newCanvas.height = 256;
var holder = document.getElementById('canvasholder');
holder.appendChild(newCanvas);

var fetch_file = function(url, cb) {
	var xhr = new XMLHttpRequest();
	xhr.open("GET", url, true);
	xhr.responseType = "arraybuffer";
	xhr.onload = function(e) {
		var ints = new Int8Array(xhr.response);
		cb(ints);
	};
	xhr.send();
};

var Module = {
	'arguments': MESS_ARGS,
	print: (function() {
		var element = document.getElementById('output');
		return function(text) {
			element.innerHTML += text + '<br>';
		};
	})(),
	canvas: document.getElementById('canvas'),
	screenIsReadOnly: true,
	noInitialRun: false,
	preInit: function() {
		// Load the downloaded binary files into the filesystem.
		for (var bios_fname in bios_files) {
			if (bios_files.hasOwnProperty(bios_fname)) {
				Module['FS_createDataFile']('/', bios_fname, bios_files[bios_fname], true, true);
			}
		}
		if (gamename !== "") {
			Module['FS_createDataFile']('/', gamename, game_file, true, true);
		}
		if (Modernizr.webaudio && !(Modernizr.mozsetup)) {
			var asample;
			try {
				asample = new AudioContext();
			} catch (e) {
				asample = new webkitAudioContext();
			}
			Module.arguments.push("-samplerate", asample.sampleRate.toString());
		}
	}
};

var update_countdown = function() {
  file_countdown -= 1;
  if (file_countdown === 0) {
    var headID = document.getElementsByTagName("head")[0];
		var newScript = document.createElement('script');
		newScript.type = 'text/javascript';
		newScript.src = 'MESS_SRC';
		headID.appendChild(newScript);
  }
};

// Fetch the BIOS and the game we want to run.
for (var i=0; i < bios_filenames.length; i++) {
  var fname = bios_filenames[i];
  if (fname === "") {
    continue;
  }
  fetch_file(fname, function(data) { bios_files[fname] = data; update_countdown(); });
}

if (gamename !== "") {
  fetch_file(gamename, function(data) { game_file = data; update_countdown(); });
}
