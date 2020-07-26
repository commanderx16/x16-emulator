
// DOM elements
const statusElement = document.getElementById('status');
const output = document.getElementById('output');
const canvas = document.getElementById('canvas');
const code = document.getElementById('code');
const progressElement = document.getElementById('progress');
const spinnerElement = document.getElementById('spinner');
const volumeElementFullScreen = document.getElementById('fullscreen_volume_icon');
const volumeElement = document.getElementById('volume_icon');


// Audio Context Setup
var audioContext;

window.addEventListener('load', init, false);
function init() {

    try {
        window.AudioContext = window.AudioContext || window.webkitAudioContext;
        audioContext = new AudioContext();
    }
    catch (e) {
        console.log("AudioContext not supported on this Browser.")
    }
}

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

if (layouts.includes(lang)) {
    logOutput('Using keyboard map: ' + lang);
} else {
    logOutput('Language (' + lang + ') not found in keymaps so using keyboard map: en-us');
    lang = 'en-us';
}

var url = new URL(window.location.href);
var manifest_link = url.searchParams.get("manifest");
var isZipFile = false;

if (manifest_link && manifest_link.endsWith('.zip')) {
    isZipFile = true;
}
else if (manifest_link && !manifest_link.endsWith('/')) {
    manifest_link = manifest_link + '/';
}

var emuArguments = ['-keymap', lang];

if (manifest_link) {
    openFs();
}

var Module = {

    preRun: [
        function () { //Set the keyboard handling element (it's document by default). Keystrokes are stopped from propagating by emscripten, maybe there's an option to disable this?
            ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT = "#canvas";
        },
        function () {

            if (manifest_link) {
                if (isZipFile === true) {
                    loadZip(manifest_link);
                }
                else {
                    loadManifest();
                }
            }
        }
    ],
    postRun: [
        function () {
            canvas.focus();
        }
    ],
    arguments: emuArguments,
    print: (function () {

        if (output) output.value = ''; // clear browser cache
        return function (text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            logOutput(text);
        };
    })(),
    printErr: function (text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');

        // filtering out some known issues for easier reporting from people who have startup problems
        if (text.startsWith('wasm streaming compile failed:') ||
            text.startsWith('falling back to ArrayBuffer instantiation') ||
            text.startsWith('Calling stub instead of sigaction')) {
            logOutput("[known behavior] " + text);
            return;
        }

        logError(text);


    },
    canvas: (function () {

        // As a default initial behavior, pop up an alert when webgl context is lost. To make your
        // application robust, you may want to override this behavior before shipping!
        // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
        canvas.addEventListener("webglcontextlost", function (e) {
            alert('WebGL context lost. You will need to reload the page.');
            e.preventDefault();
        }, false);
        return canvas;
    })(),
    setStatus: function (text) {
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
    monitorRunDependencies: function (left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    }
};

Module.setStatus('Downloading file...');
logOutput('Downloading file...');

window.onerror = function () {
    // Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function (text) {
        if (text) Module.printErr('[post-exception status] ' + text);
    };
};

function loadZip(zipFileUrl) {
    addRunDependency('load-zip');
    fetch(zipFileUrl)
        .then(function (response) {
            if (response.status === 200 || response.status === 0) {
                return Promise.resolve(response.blob());
            } else {
                return Promise.reject(new Error(response.statusText));
                // todo error handling here, display to user
            }
        })
        .then(JSZip.loadAsync)
        .then(extractManifestFromBuffer)
        .then(function () {
            console.log("Starting Emulator...")
            console.log("Emulator arguments: ", emuArguments)
            removeRunDependency('load-zip');
        });
}

function extractManifestFromBuffer(zip) {
    if (zip.file("manifest.json") == null) {
        logError("Unable to find manifest.json within: " + manifest_link);
        return Promise.resolve();
    }
    else {
        return zip.file("manifest.json").async("uint8array")
            .then(function (content) {
                let manifestString = new TextDecoder("utf-8").decode(content);
                let manifestObject = JSON.parse(manifestString);
                console.log("Parsed manifest from zip:")
                console.log(manifestObject);

                if (manifestObject.start_bas && manifestObject.start_prg) {
                    logError("start_bas and start_prg used in manifest");
                    logError("This is likely an error, defaulting to start_bas")
                }

                if (manifestObject.start_bas) {
                    console.log('Adding start BAS:', manifestObject.start_bas)
                    if (!manifestObject.resources.includes(manifestObject.start_bas)) {
                        logError("start_bas not found within resources entries");
                    }
                    else {
                        emuArguments.push('-bas', manifestObject.start_bas, '-run');
                    }

                }
                else if (manifestObject.start_prg) {
                    console.log('Adding start PRG: ', manifestObject.start_prg)
                    if (!manifestObject.resources.includes(manifestObject.start_prg)) {
                        logError("start_prg not found within resources entries");
                    }
                    else {
                        emuArguments.push('-prg', manifestObject.start_prg, '-run');
                    }
                }

                let promises = [];
                manifestObject.resources.forEach(function (element) {
                    let fileName = element.replace(/^.*[\\\/]/, '');

                    if (zip.file(fileName) == null) {
                        logError("Unable to find resources entry: " + fileName);
                        logError("This is likely an error, check resources section in manifest.")
                    } else {
                        promises.push(zip.file(fileName).async("uint8array").then(function (content) {
                            console.log('Writing to emulator filesystem:', fileName);
                            FS.writeFile(fileName, content);
                        }));
                    }
                });
                return Promise.all(promises);
            })
            .then((value) => {
                console.log("Emulator filesystem loading complete.")
            });
    }
}

function loadManifest() {
    addRunDependency('load-manifest');
    fetch(manifest_link + 'manifest.json').then(function (response) {
        return response.json();
    }).then(function (manifest) {
        if (manifest.start_bas) {
            emuArguments.push('-bas', manifest.start_bas, '-run');
        }
        else if (manifest.start_prg) {
            console.log('Adding start PRG: ', manifest.start_prg)
            emuArguments.push('-prg', manifest.start_prg, '-run');
        }
        console.log("Loading from manifest:")
        console.log(manifest);
        manifest.resources.forEach(element => {
            element = manifest_link + element;
            let filename = element.replace(/^.*[\\\/]/, '')
            FS.createPreloadedFile('/', filename, element, true, true);

        });
        removeRunDependency('load-manifest');
    }).catch(function () {
        console.log("Unable to read manifest. Check the manifest http parameter");
    });
}

function toggleAudio() {
    if (audioContext && audioContext.state != "running") {
        audioContext.resume().then(() => {
            volumeElement.innerHTML = "volume_up";
            volumeElementFullScreen.innerHTML = "volume_up";
            console.log("Resumed Audio.")
            Module.ccall("j2c_start_audio", "void", ["bool"], [true]);
        });
    } else if (audioContext && audioContext.state == "running") {
        audioContext.suspend().then(function () {
            console.log("Stopped Audio.")
            volumeElement.innerHTML = "volume_off";
            volumeElementFullScreen.innerHTML = "volume_off";
            Module.ccall("j2c_start_audio", "void", ["bool"], [false]);
        });
    }
    canvas.focus();
}

function resetEmulator() {
    j2c_reset = Module.cwrap("j2c_reset", "void", []);
    j2c_reset();
    canvas.focus();
}

function runCode() {
    Module.ccall("j2c_paste", "void", ["string"], ['\nNEW\n' + code.value + '\nRUN\n']);
    canvas.focus();
}

function closeFs() {
    canvas.parentElement.classList.remove("fullscreen");
    canvas.focus();
}

function openFs() {
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

function logError(text) {
    if (output) {
        output.innerHTML += "[error] " + text + "\n";
        output.parentElement.scrollTop = output.parentElement.scrollHeight; // focus on bottom
    }
    console.error(text);
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
