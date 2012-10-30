// connect to canvas
//
//<canvas id='canvas' width='256' height='256'></canvas>

var newCanvas = document.createElement('canvas');
newCanvas.id = 'canvas';
newCanvas.width = 256;
newCanvas.height = 256;
var holder = document.getElementById('canvasholder');
holder.appendChild(newCanvas);
var url = $.url();
var gamename = url.param('gamename') || 'GAME_FILE';
var gameurl = url.param('gameurl') || 'GAME_FILE';

var Module = {
	arguments: ['a800','-verbose','-rompath','.','-cart1',gamename,'-window','-resolution','336x225','-nokeepaspect'],
	print: (function() {
		var element = document.getElementById('output');
		return function(text) {
			text = text.replace(/&/g, "&amp;");
			text = text.replace(/</g, "&lt;");
			text = text.replace(/>/g, "&gt;");
			text = text.replace('\n', '<br>', 'g');
			element.innerHTML += text + '<br>';
		};
	})(),
	canvas: document.getElementById('canvas'),
	noInitialRun: false,
	preRun: function() {
		var ajaxArgs = {
			url: gameurl,
			beforeSend: function ( xhr ) {
				xhr.overrideMimeType("text/plain; charset=x-user-defined");
			},
			dataType: 'text'
		}
		$.when($.ajax(ajaxArgs)).done(function(data) {
			if (typeof data === 'string') {
				var dataArray = new Array(data.length);
				for (var i = 0, len = data.length; i < len; ++i) dataArray[i] = data.charCodeAt(i);
				data = dataArray;
			}

			FS.createDataFile('/', gamename, data, true, false);
			try {
				if ("undefined" !== typeof removeRunDependency) {
					removeRunDependency();
				}
				else {
					Module.run();
				}
			} catch (e) {

			}

	  });
	}

};

//this is a gross hack but is enough for a pretty demo
function runTimeslice($this) {
	runTimesliceWorker($this);
	_abort(0);
}

function runTimesliceWorker($this) {
	__ZN16device_scheduler9timesliceEv($this);
	setTimeout(function() { runTimesliceWorker($this) }, 1);
}

// The compiled code

//Need to edit __ZN15running_machine3runEb() - replace call to __ZN16device_scheduler9timesliceEv() with runTimeslice()

var headID = document.getElementsByTagName("head")[0];
var newScript = document.createElement('script');
newScript.type = 'text/javascript';
newScript.src = 'MESS_SRC';
headID.appendChild(newScript);

