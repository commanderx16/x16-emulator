
// DOM elements
const statusElement = document.getElementById('status');
const progressElement = document.getElementById('progress');
const spinnerElement = document.getElementById('spinner');
const output = document.getElementById('output');
const canvas = document.getElementById('canvas');
const code = document.getElementById('code');

//detecting keyboard layout...

//define valid layouts (this can be gotten by running the emulator with -keymap)
const layouts = [
    'en-us',
    'en-gb',
    'de',
    'nordic',
    'it',
    'pl',
    'hu',
    'es',
    'fr',
    'de-ch',
    'fr-be',
    'pt-br'
];

lang = getFirstBrowserLanguage().toLowerCase().trim();

if(layouts.includes(lang)) {
    logOutput('Using keyboard map: ' + lang);
} else {
    logOutput('Language ('+lang+') not found in keymaps so using keyboard map: en-us');
    lang = 'en-us';
}



Module = {
    preRun: [
        function() {         //Set the keyboard handling element (it's document by default). Keystrokes are stopped from propagating by emscripten, maybe there's an option to disable this?
            ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT = "#canvas";
        }
    ],
    postRun: [
        function () {
            canvas.focus();
        }
    ],
    arguments: [    //set key map to user's lang
        '-keymap',lang
    ],
    print: (function() {

        if (output) output.value = ''; // clear browser cache
        return function(text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            logOutput(text);
        };
    })(),
    printErr: function(text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');

        logOutput("[error] " + text);


    },
    canvas: (function() {

        // As a default initial behavior, pop up an alert when webgl context is lost. To make your
        // application robust, you may want to override this behavior before shipping!
        // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
        canvas.addEventListener("webglcontextlost", function(e) {
            alert('WebGL context lost. You will need to reload the page.');
            e.preventDefault();
        }, false);
        return canvas;
    })(),
    setStatus: function(text) {
        if (!Module.setStatus.last) Module.setStatus.last = {
            time: Date.now(),
            text: ''
        };
        if (text === Module.setStatus.last.text) return;
        const m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        let now = Date.now();
        if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
        Module.setStatus.last.time = now;
        Module.setStatus.last.text = text;
        if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2]) * 100;
            progressElement.max = parseInt(m[4]) * 100;
            progressElement.hidden = false;
            spinnerElement.hidden = false;
        } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.hidden = true;
        }
        statusElement.innerHTML = text;
        logOutput(text);
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    }
};




Module.setStatus('Downloading file...');
logOutput('Downloading file...');

window.onerror = function() {
    Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function(text) {
        if (text) Module.printErr('[post-exception status] ' + text);
    };
};



function resetEmulator() {
    j2c_reset = Module.cwrap("j2c_reset", "void", []);
    j2c_reset();
    canvas.focus();
}

function runCode() {

    Module.ccall("j2c_paste", "void", ["string"], ['\nNEW\n'+ code.value + '\nRUN\n']);
    canvas.focus();

}

function closeFs(){
    canvas.parentElement.classList.remove("fullscreen");
    canvas.focus();
}

function openFs(){
    canvas.parentElement.classList.add("fullscreen");
    canvas.focus();
}

function logOutput(text) {
    if (output) {
        output.innerHTML += text + "\n";
        output.parentElement.scrollTop = output.parentElement.scrollHeight; // focus on bottom
    }
    console.log(text);
}


function getFirstBrowserLanguage() {
    const nav = window.navigator,
        browserLanguagePropertyKeys = ['language', 'browserLanguage', 'systemLanguage', 'userLanguage'];
    let i,
        language;

    // support for HTML 5.1 "navigator.languages"
    if (Array.isArray(nav.languages)) {
        for (i = 0; i < nav.languages.length; i++) {
            language = nav.languages[i];
            if (language && language.length) {
                return language;
            }
        }
    }

    // support for other well known properties in browsers
    for (i = 0; i < browserLanguagePropertyKeys.length; i++) {
        language = nav[browserLanguagePropertyKeys[i]];
        if (language && language.length) {
            return language;
        }
    }

    return null;
}

