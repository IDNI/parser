# tau_wasm_terminal_page(<page_dir>) - copy the generic terminal page files
# and the vendored xterm.js files into a WASM page output directory.
function(tau_wasm_terminal_page page_dir)
    set(_pkg_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

    foreach(_f terminal.js sw.js shell.css)
        configure_file(${_pkg_dir}/${_f} ${page_dir}/${_f} COPYONLY)
    endforeach()

    set(_node_modules ${_pkg_dir}/node_modules)
    if(NOT EXISTS ${_node_modules})
        message(FATAL_ERROR
            "tau_wasm_terminal_page: ${_node_modules} not found - run "
            "'npm ci --prefix js/tau-wasm-terminal' first")
    endif()

    file(COPY ${_node_modules}/@xterm/xterm/lib/xterm.js
         DESTINATION ${page_dir}/vendor/)
    file(COPY ${_node_modules}/@xterm/xterm/css/xterm.css
         DESTINATION ${page_dir}/vendor/)
    file(COPY ${_node_modules}/@xterm/addon-fit/lib/addon-fit.js
         DESTINATION ${page_dir}/vendor/)
    file(RENAME ${page_dir}/vendor/addon-fit.js
         ${page_dir}/vendor/xterm-addon-fit.js)
    file(COPY ${_node_modules}/@xterm/addon-webgl/lib/addon-webgl.js
         DESTINATION ${page_dir}/vendor/)
    file(RENAME ${page_dir}/vendor/addon-webgl.js
         ${page_dir}/vendor/xterm-addon-webgl.js)
endfunction()
