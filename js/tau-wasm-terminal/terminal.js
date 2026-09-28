// terminal.js - xterm.js glue for hosting an Emscripten terminal program.
// Entry point: tau_wasm_terminal.start(options) returns { term, send,
// write_file }, or null while the COOP/COEP service worker reloads the page.
var tau_wasm_terminal = (function() {
'use strict';

function start(options) {
  // COOP/COEP via ServiceWorker - required for SharedArrayBuffer (pthreads)
  if ('serviceWorker' in navigator && !window.crossOriginIsolated) {
    var url_sw = new URL('./sw.js', location.href);
    navigator.serviceWorker.register(url_sw).then(function() { location.reload(); });
    return null;
  }

  var term = new Terminal();
  term.options.scrollback = 1000;
  term.options.convertEol = true; // program output uses bare \n; map to \r\n
  term.options.fontFamily =
    "'DejaVu Sans Mono', 'Liberation Mono', 'Noto Sans Mono', monospace";
  term.open(options.element);

  var fit_addon = new FitAddon.FitAddon();
  term.loadAddon(fit_addon);

  // GPU rendering opt-in: ?webgl - falls back to DOM on failure/context loss
  if (new URLSearchParams(location.search).has('webgl')) {
    try {
      var webgl_addon = new WebglAddon.WebglAddon();
      webgl_addon.onContextLoss(function() { webgl_addon.dispose(); });
      term.loadAddon(webgl_addon);
    } catch (e) {
      console.error('WebGL renderer unavailable, using DOM renderer:', e);
    }
  }

  term.resize(140, 43);

  var stdin_buffer = [];
  // stdout and stderr share this queue so xterm.js sees both streams in the
  // order the program wrote them, not reordered by which one hits a newline first.
  var out_queue = [];

  var stdin = function() { return stdin_buffer.shift() || 0; };

  function flush_out_queue() {
    term.write(new Uint8Array(out_queue));
    out_queue.length = 0;
  }

  var push_out = function(code) {
    if (code == 0) return;
    if (out_queue.length == 0) setTimeout(flush_out_queue, 0);
    out_queue.push(code);
  };

  var send = function(text) {
    for (var i = 0; i < text.length; i++) stdin_buffer.push(text.charCodeAt(i));
  };

  term.onBinary(send);
  term.onData(send);
  window.term = term;
  window.sendReplInput = send;

  window.Module = {
    preRun: [],
    arguments: options.arguments || [],
    onRuntimeInitialized: function() {
      if (window.Module._ftxui_on_resize === undefined) return;
      fit_addon.fit();
      var resize_handler = function() {
        var dims = fit_addon.proposeDimensions();
        term.resize(dims.cols, dims.rows);
        window.Module._ftxui_on_resize(dims.cols, dims.rows);
        fit_addon.fit();
      };
      var resize_observer = new ResizeObserver(resize_handler);
      resize_observer.observe(options.element);
      resize_handler();
    },
  };

  // preRun must be an array BEFORE the WASM script loads,
  // because Emscripten does preRun.push(runWithFS) for --preload-file.
  window.Module.preRun.push(function() {
    FS.init(stdin, push_out, push_out);
  });

  var script = document.createElement('script');
  script.src = options.program;
  document.head.appendChild(script);

  return {
    term: term,
    send: send,
    write_file: function(path, bytes) {
      if (typeof FS === 'undefined') throw new Error('FS not ready');
      FS.writeFile(path, bytes);
    },
  };
}

return { start: start };

})();

// A classic page script already exposes this as a global; assign it
// explicitly so the boundary does not depend on script-vs-module loading.
if (typeof window !== 'undefined') window.tau_wasm_terminal = tau_wasm_terminal;
