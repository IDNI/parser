// node.js - Node host for an Emscripten terminal program.
// Usage: node node.js <program.js> [program arguments...]
'use strict';

var fs = require('fs');
var path = require('path');
var vm = require('vm');

function start(program, args) {
  var caller_dir = process.cwd();
  var program_path = path.resolve(program);

  var stdin_buffer = [];
  // stdout and stderr share this queue so the terminal sees both streams in
  // the order the program wrote them, not reordered by which one hits a newline first.
  var out_queue = [];

  // FTXUI reads stdin until it gets byte 0, so an empty buffer must give 0,
  // not null (end of file) or undefined (EAGAIN) which leave its loop
  // spinning; this host never reports end of file.
  var stdin = function() { return stdin_buffer.shift() || 0; };

  function flush_out_queue() {
    process.stdout.write(Buffer.from(out_queue));
    out_queue.length = 0;
  }

  var push_out = function(code) {
    if (code == 0) return;
    if (out_queue.length == 0) setTimeout(flush_out_queue, 0);
    out_queue.push(code);
  };

  var send = function(bytes) {
    for (var i = 0; i < bytes.length; i++) stdin_buffer.push(bytes[i]);
  };

  var restore_terminal = function() {
    if (process.stdin.isTTY) process.stdin.setRawMode(false);
  };

  if (process.stdin.isTTY) process.stdin.setRawMode(true);
  process.stdin.on('data', send);
  process.on('exit', function() {
    if (out_queue.length) flush_out_queue();
    restore_terminal();
  });

  // an absolute path that exists only in the virtual file system stays as is
  var host_path = function(arg) {
    return path.isAbsolute(arg) && fs.existsSync(arg) ? '/host' + arg : arg;
  };

  global.Module = {
    preRun: [],
    arguments: args.map(host_path),
    onRuntimeInitialized: function() {
      if (global.Module._ftxui_on_resize === undefined) return;
      var resize_handler = function() {
        global.Module._ftxui_on_resize(process.stdout.columns || 80,
                                       process.stdout.rows || 24);
      };
      process.stdout.on('resize', resize_handler);
      resize_handler();
    },
    onExit: function(status) {
      if (out_queue.length) flush_out_queue();
      restore_terminal();
      process.exit(status);
    },
  };

  // preRun must be an array BEFORE the WASM script loads,
  // because Emscripten does preRun.push(runWithFS) for --preload-file.
  global.Module.preRun.push(function() {
    FS.init(stdin, push_out, push_out);
    // relative arguments then resolve against the caller's directory, as typed
    FS.mkdir('/host');
    FS.mount(NODEFS, { root: '/' }, '/host');
    FS.chdir('/host' + caller_dir);
  });

  // the .data preload file loads relative to the current directory
  process.chdir(path.dirname(program_path));
  // run as a classic script, like a page script tag: under require the
  // program's own var Module would shadow global.Module and hide FS
  global.require = require;
  global.__filename = program_path;
  global.__dirname = path.dirname(program_path);
  vm.runInThisContext(fs.readFileSync(program_path, 'utf8'),
                      { filename: program_path });
}

if (process.argv.length < 3) {
  process.stderr.write('usage: node node.js <program.js> [program arguments...]\n');
  process.exit(2);
}
start(process.argv[2], process.argv.slice(3));
