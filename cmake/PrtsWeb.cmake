# Emscripten (web) build of the launcher: 1stprts.html/.js/.wasm with the shell page and JS glue
# from web/, plus data/ packed separately into 1stprts.data (+ its loader 1stprts.data.js).
# See docs/architecture/data-and-scripting.md §1 and design D2, D13, D14 of add-engine-foundation.

set(PRTS_WEB_DIR ${CMAKE_SOURCE_DIR}/web)

function(prts_web_executable target)
  set(web ${PRTS_WEB_DIR})
  set_target_properties(${target} PROPERTIES SUFFIX ".html")

  # C functions the JS glue calls (web/prts_pre.js, web/prts_xr.js, web/prts_test_hooks.js).
  set(exports _main _prts_frame _prts_web_set_touch_first _prts_web_user_gesture
      _prts_xr_web_frame_buffer _prts_xr_web_frame _prts_xr_web_set_supported
      _prts_xr_web_session_started _prts_xr_web_session_ended)
  set(runtime "")
  set(pre_js ${web}/prts_pre.js)
  if(PRTS_WEB_TEST_HOOKS)
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/web_hooks.c)
    target_compile_definitions(${target} PRIVATE PRTS_WEB_TEST_HOOKS=1)
    list(APPEND exports _prts_test_state _prts_test_frames _prts_test_commander _prts_test_scheme
         _prts_test_gestures _prts_test_xr _prts_test_user_write _prts_test_user_read
         _prts_test_scenario _prts_test_scenario_text _prts_test_audio _prts_test_ui_has)
    set(runtime ccall UTF8ToString)
    list(APPEND pre_js ${web}/prts_test_hooks.js)
  endif()
  list(JOIN exports "," exports)

  set(opts
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2  # WebGL 2 only (OpenGL ES 3.0)
    -sALLOW_MEMORY_GROWTH=1
    -sSTACK_SIZE=1MB                             # Lua and the boot path need more than the 64 KB default
    -sENVIRONMENT=web
    -sFORCE_FILESYSTEM=1                         # the data package is loaded by a separate script
    -lidbfs.js                                   # /user lives in IndexedDB
    "-sEXPORTED_FUNCTIONS=${exports}"
    "SHELL:--shell-file ${web}/shell.html"
    "SHELL:--js-library ${web}/prts_web.js"
    "SHELL:--js-library ${web}/prts_xr.js")
  foreach(js ${pre_js})
    list(APPEND opts "SHELL:--pre-js ${js}")
  endforeach()
  if(runtime)
    list(JOIN runtime "," runtime)
    list(APPEND opts "-sEXPORTED_RUNTIME_METHODS=${runtime}")
  endif()
  target_link_options(${target} PRIVATE ${opts})
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    ${web}/shell.html ${web}/prts_web.js ${web}/prts_xr.js ${pre_js})

  # data/ -> 1stprts.data, mounted at /data. A separate download that can be rebuilt without
  # relinking the .wasm (the shell page loads 1stprts.data.js before 1stprts.js).
  set(out ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  file(GLOB_RECURSE data_files CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/data/*)
  set(packs --preload ${CMAKE_SOURCE_DIR}/data@/data)
  if(PRTS_WEB_TEST_HOOKS)
    # Test builds also carry the automated game tests, so the browser tests can run them
    # (?test=1&scenario=<name>, task 10.5). Shipped builds have only data/.
    file(GLOB scenario_files CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/game/scenarios/*.lua)
    list(APPEND data_files ${scenario_files})
    list(APPEND packs --preload ${CMAKE_SOURCE_DIR}/tests/game/scenarios@/tests/scenarios)
  endif()
  file(MAKE_DIRECTORY ${out})
  add_custom_command(
    OUTPUT ${out}/1stprts.data ${out}/1stprts.data.js
    COMMAND ${Python3_EXECUTABLE} ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager.py 1stprts.data
            ${packs} --js-output=1stprts.data.js --no-node --quiet
    WORKING_DIRECTORY ${out}
    DEPENDS ${data_files}
    COMMENT "Packing data/ into 1stprts.data"
    VERBATIM)
  add_custom_target(prts_web_data ALL DEPENDS ${out}/1stprts.data ${out}/1stprts.data.js)
  add_dependencies(${target} prts_web_data)
endfunction()
