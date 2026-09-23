# tau-wasm-terminal

Hosts for an Emscripten terminal program: xterm.js glue for a browser page,
and a Node.js host for the terminal.

A page calls `tau_wasm_terminal.start({ element, program, arguments })` and
gets back `{ term, send, write_file }`.

A CMake project runs `include(tau-wasm-terminal.cmake)` then
`tau_wasm_terminal_page(<page_dir>)` to copy the page and vendor files.

Serve a built page with `node serve.js <page_dir> [port]`.

Run a built program in the terminal with `node node.js <program.js> [args...]`.
