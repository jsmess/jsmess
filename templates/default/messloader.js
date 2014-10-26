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
		JSMESS._readyList.push(JSMESS.setScale);
		if (!(JSMESS._readySet)) JSMESS._readyCheck();
	};
};
JSMESS.setScale = function() {
	Module.canvas.style.width = Module.canvas.width * scale + 'px';
	Module.canvas.style.height = Module.canvas.height * scale + 'px';
};
JSMESS.fullScreenChangeHandler = function() {
	if (!(document.mozFullScreenElement || document.fullScreenElement)) {
		setTimeout(JSMESS.setScale, 0);
	}
}


var gamename = 'GAME_FILE';
var game_file = null;
var bios_filenames = 'BIOS_FILES'.split(' ');
var bios_files = {};
var file_countdown = 0;
var scale = 2;
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

var fullscreenbutton = document.getElementById('gofullscreen');
if (fullscreenbutton) {
	fullscreenbutton.addEventListener('click', gofullscreen);
	if ('onfullscreenchange' in document) {
		document.addEventListener('fullscreenchange', JSMESS.fullScreenChangeHandler);
	} else if ('onmozfullscreenchange' in document) {
		document.addEventListener('mozfullscreenchange', JSMESS.fullScreenChangeHandler);
	} else if ('onwebkitfullscreenchange' in document) {
		document.addEventListener('webkitfullscreenchange', JSMESS.fullScreenChangeHandler);
	}
}

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
	SDL_numSimultaneouslyQueuedBuffers: 5,
	noInitialRun: false,
	screenIsReadOnly: true,
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

function gofullscreen() {
  Module.requestFullScreen(1,0);
}

// Fetch the BIOS and the game we want to run.
for (var i=0; i < bios_filenames.length; i++) {
  var fname = bios_filenames[i];
  if (fname === "") {
    continue;
  }
  function getFunction(fname) {
     // Wrapper function to avoid binding fname to loop variable
     return function(data) { bios_files[fname] = data; update_countdown(); }
  }
  fetch_file(fname, getFunction(fname));
}

if (gamename !== "") {
  fetch_file(gamename, function(data) { game_file = data; update_countdown(); });
}
