// Layer to be called from the C/C++

// C  -> Javascript
// Functions  that can be called from the native side

mergeInto(LibraryManager.library, {
  my_js: function() {
    console.log("my_js called from C");
    // dummy function just for demonstration purpose
  }
});

// Javascript -> C
//Functions that can call the native side

int_sqrt = Module.cwrap("int_sqrt", "number", ["number"]);
