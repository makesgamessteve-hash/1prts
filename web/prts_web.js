/* 1stPRTS web glue: JS functions the engine's C code calls (Emscripten --js-library).
 * The C declarations are in engine/src/platform/web_glue.h. MIT License. */
addToLibrary({
  // Errors that would be a dialog on desktop are shown on the page (web/shell.html).
  prts_web_show_error__deps: ['$UTF8ToString'],
  prts_web_show_error: function (title, message) {
    var t = UTF8ToString(title);
    var m = UTF8ToString(message);
    console.error(t + ': ' + m);
    if (Module['prtsShowError']) Module['prtsShowError'](t, m);
  },
});
