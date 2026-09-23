# tau-wasm-terminal

xterm.js glue for hosting an Emscripten terminal program in a browser page.

A page calls `tau_wasm_terminal.start({ element, program, arguments })` and
gets back `{ term, send, write_file }`.

A CMake project runs `include(tau-wasm-terminal.cmake)` then
`tau_wasm_terminal_page(<page_dir>)` to copy the page and vendor files.

Serve a built page with `node serve.js <page_dir> [port]`.
