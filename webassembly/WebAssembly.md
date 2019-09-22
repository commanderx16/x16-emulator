## WebAssembly/HTML5 Support
The Commander X16 emulator supports a HTML/WebAssembly build target using the [Emscripten compiler](https://emscripten.org/).

## Demo
[HTML X16 Emulator Demo](https://sebastianvog.github.io/x16-emulator/x16emu.html)
## Installation
Follow installation steps from [here](https://emscripten.org/docs/getting_started/downloads.html#)

## Build

    make wasm

This outputs the following artifacts in the build directory, which can be uploaded to any web server.

	ex16mu.data x16emu.html x16emu.js x16mu.wasm x16emuworker.js

### Testing
To run a test webserver:

	python -m SimpleHTTPServer 8080

And start [http://localhost:8080/x16emu.html](http://localhost:8080/x16emu.html)

Note: You will get the error `wasm streaming compile failed: TypeError: Failed to execute 'compile' on 'WebAssembly'...` 
This is because SimpleHTTPServer does not serve .wasm files with the correct mime type. don't worry, everything will still work fine though.

### Supported Browsers
Working on the latest versions of:
* Chrome (including Android)
* Edge
* Firefox

IE and Safari are not supported.