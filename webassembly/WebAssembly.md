## WebAssembly/HTML5 Support
The Commander X16 emulator supports a HTML/WebAssembly build target using the [Emscripten compiler](https://emscripten.org/).

## Demo
[HTML X16 Emulator Demo](https://sebastianvog.github.io/x16-emulator/x16emu.html)
## Installation
Follow installation steps from [here](https://emscripten.org/docs/getting_started/downloads.html#)

## Build

    emmake make

This outputs the following artifacts in the build directory, which can be uploaded to any web server.

	ex16mu.data x16emu.html x16emu.js x16mu.wasm x16emuworker.js

### Testing
To run a test webserver:

	python -m SimpleHTTPServer 8080

And start [http://localhost:8080/x16emu.html](http://localhost:8080/x16emu.html)

### Supported Browsers
Currently supported Browser is Google Chrome only.

FireFox will work if SharedMemory is enabled from preferences (but comes with Security implications).

Safari is not supported.