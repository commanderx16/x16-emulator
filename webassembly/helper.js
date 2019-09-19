// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD


/********** C  -> Javascript **********/
// Functions exposed by the Javascript runtime that can be called from the native Emulator

mergeInto(LibraryManager.library, {
    my_js: function() {
        console.log("my_js called from C");
        // dummy function just for demonstration purpose
    }
});


/********** Javascript -> C **********/
// Functions exposed by the native emulator that can be calle from Javascript


// Paste the current clipboard to the emulator
j2c_paste = Module.cwrap("j2c_paste", "void", ["string"]);

// Reset the emulator
j2c_reset = Module.cwrap("j2c_reset", "void", []);