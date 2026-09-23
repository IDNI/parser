// TGF browser REPL - wires tau_wasm_terminal to the TGF WASM program.
(function() {
'use strict';

var statusEl = document.getElementById('status');

var terminal = tau_wasm_terminal.start({
  element: document.querySelector('#terminal'),
  program: 'tgf_standalone.js',
  arguments: ['/tau.tgf'],
});
if (!terminal) return; // service worker reload in progress

statusEl.textContent = 'grammar: /tau.tgf';

// --- Grammar file upload ---

var upload = document.getElementById('upload');
upload.addEventListener('change', function(e) {
  var file = e.target.files[0];
  if (!file) return;
  var path = '/' + file.name;
  var reader = new FileReader();
  reader.onload = function() {
    try {
      terminal.write_file(path, new Uint8Array(reader.result));
    } catch (err) {
      statusEl.textContent = 'WASM not ready, retrying...';
      setTimeout(function() { reader.onload(); }, 100);
      return;
    }
    statusEl.textContent = 'loading: ' + path;
    terminal.send('load "' + path + '"\n');
  };
  reader.readAsArrayBuffer(file);
});

})();
