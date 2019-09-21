## WebAssembly/HTML5 Support
The Commander X16 emulator supports a HTML/WebAssembly build target using the [Emscripten compiler](https://emscripten.org/).

## Demo
[HTML X16 Emulator Demo](https://sebastianvog.github.io/x16-emulator/x16emu.html)
## Installation
Follow installation steps from [here](https://emscripten.org/docs/getting_started/downloads.html#)

## Build

    make wasm

This outputs the following artifacts in the build directory, which can be uploaded to any web server.

	ex16mu.data x16emu.html x16emu.js x16mu.wasm x16emuworker.js webassembly/styles.css webassembly/heper.js

### Testing
To run a test webserver:

	python -m SimpleHTTPServer 8080

And start [http://localhost:8080/x16emu.html](http://localhost:8080/x16emu.html)

### Supported Browsers
Working Browsers are Chrome, Safari, FireFox, Opera and Microsoft Edge (Beta)

### Known Issues
* Speed of the webassembly emulation is lacking, compared to the native version
* Copy Clipboard button only works on Chrome
* Ctrl-V is not working
* Resizing doesn't work all that well
* No support to load PRGs
* General keyboard input support on Mobile browsers
   