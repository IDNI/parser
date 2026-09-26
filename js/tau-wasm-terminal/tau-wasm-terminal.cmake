# Shared terminal page assets for an Emscripten terminal program.
#
# tau_wasm_terminal_page(<page_dir>
#     [PACKAGE_DIR <node_modules_dir>]
#     [ADDON_NAMES <rename|keep>])
#
# Copies the page glue and the vendored xterm.js assets. PACKAGE_DIR defaults
# to this directory's own node_modules. ADDON_NAMES=keep (the default) keeps
# the author's npm filenames addon-fit.js / addon-webgl.js next to the
# vendored xterm.js; ADDON_NAMES=rename writes xterm-addon-fit.js /
# xterm-addon-webgl.js instead. A page's <script src> names must match the
# chosen policy.
function(tau_wasm_terminal_page page_dir)
    cmake_parse_arguments(ARG "" "PACKAGE_DIR;ADDON_NAMES" "" ${ARGN})
    if(NOT ARG_PACKAGE_DIR)
        set(ARG_PACKAGE_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/node_modules")
    endif()
    if(NOT ARG_ADDON_NAMES)
        set(ARG_ADDON_NAMES "keep")
    endif()
    if(NOT ARG_ADDON_NAMES STREQUAL "rename"
       AND NOT ARG_ADDON_NAMES STREQUAL "keep")
        message(FATAL_ERROR
            "tau_wasm_terminal_page: ADDON_NAMES must be 'rename' or 'keep', "
            "got '${ARG_ADDON_NAMES}'")
    endif()
    tau_wasm_terminal_page_files("${page_dir}")
    tau_wasm_terminal_vendor("${page_dir}" "${ARG_PACKAGE_DIR}" "${ARG_ADDON_NAMES}")
endfunction()

# tau_wasm_terminal_page_files(<page_dir>) - the JS/CSS page glue only, no
# vendored dependencies. Call this and tau_wasm_terminal_vendor() separately
# when a consumer vendors xterm.js from its own package tree.
function(tau_wasm_terminal_page_files page_dir)
    set(_pkg_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    foreach(_f terminal.js sw.js shell.css)
        configure_file(${_pkg_dir}/${_f} ${page_dir}/${_f} COPYONLY)
    endforeach()
endfunction()

# tau_wasm_terminal_vendor(<page_dir> <node_modules_dir> <rename|keep>) -
# the vendored xterm.js, its CSS and the fit/webgl addons.
function(tau_wasm_terminal_vendor page_dir node_modules_dir addon_names)
    if(NOT EXISTS ${node_modules_dir})
        message(FATAL_ERROR
            "tau_wasm_terminal_vendor: ${node_modules_dir} not found - run "
            "'npm ci --prefix <package dir>' first")
    endif()

    file(COPY ${node_modules_dir}/@xterm/xterm/lib/xterm.js
         DESTINATION ${page_dir}/vendor/)
    file(COPY ${node_modules_dir}/@xterm/xterm/css/xterm.css
         DESTINATION ${page_dir}/vendor/)
    file(COPY ${node_modules_dir}/@xterm/addon-fit/lib/addon-fit.js
         DESTINATION ${page_dir}/vendor/)
    file(COPY ${node_modules_dir}/@xterm/addon-webgl/lib/addon-webgl.js
         DESTINATION ${page_dir}/vendor/)

    if(addon_names STREQUAL "rename")
        file(RENAME ${page_dir}/vendor/addon-fit.js
             ${page_dir}/vendor/xterm-addon-fit.js)
        file(RENAME ${page_dir}/vendor/addon-webgl.js
             ${page_dir}/vendor/xterm-addon-webgl.js)
    endif()
endfunction()
