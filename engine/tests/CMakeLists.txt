file(GLOB PRTS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_executable(prts_tests ${PRTS_TEST_SOURCES})
target_link_libraries(prts_tests PRIVATE prts prts_utest)
target_compile_definitions(prts_tests PRIVATE
  PRTS_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
  PRTS_TEST_DATA="${CMAKE_SOURCE_DIR}/data")
prts_set_coverage(prts_tests)
add_test(NAME prts_unit_tests COMMAND prts_tests)

# Lua unit tests and automated game tests run inside the real executable, headless.
set(PRTS_LUA_COV_DIR ${CMAKE_BINARY_DIR}/lua-coverage)
function(prts_lua_cov_args out name)
  if(PRTS_COVERAGE)
    set(${out} --lua-coverage ${PRTS_LUA_COV_DIR}/${name}.info PARENT_SCOPE)
  else()
    set(${out} "" PARENT_SCOPE)
  endif()
endfunction()

prts_lua_cov_args(cov lua_unit_tests)
add_test(NAME lua_unit_tests COMMAND 1stprts --headless --test-lua --user ${CMAKE_BINARY_DIR}/test-user/lua ${cov})
file(GLOB PRTS_SCENARIOS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/game/scenarios/*.lua)
foreach(scenario ${PRTS_SCENARIOS})
  get_filename_component(name ${scenario} NAME_WE)
  prts_lua_cov_args(cov scenario_${name})
  add_test(NAME scenario_${name}
    COMMAND 1stprts --headless --scenario ${scenario} --user ${CMAKE_BINARY_DIR}/test-user/${name} ${cov})
endforeach()
target_compile_definitions(prts_tests PRIVATE PRTS_TEST_TMP="${CMAKE_BINARY_DIR}/test-tmp")

# Rendered run of the tour (needs a display; CI runs ctest under xvfb-run with Mesa llvmpipe).
add_test(NAME render_tour
  COMMAND 1stprts --size 960x540 --scenario ${CMAKE_SOURCE_DIR}/tests/game/scenarios/tour.lua
          --out ${CMAKE_BINARY_DIR}/screenshots --user ${CMAKE_BINARY_DIR}/test-user/render)
set_tests_properties(render_tour PROPERTIES LABELS "gl")
target_include_directories(prts_tests PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)

# Render golden images (spec: Visual regression): render fixed views and compare them with
# tests/golden/ref (8 levels per channel, at most 0.5% of pixels). Differences go to
# golden-diff/. A second test proves that a changed fog color is caught.
add_executable(prts_golden_compare ${CMAKE_SOURCE_DIR}/tools/golden/compare.c)
target_link_libraries(prts_golden_compare PRIVATE prts)
target_include_directories(prts_golden_compare PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)
prts_set_warnings(prts_golden_compare)
set(PRTS_GOLDEN_ARGS -DEXE=$<TARGET_FILE:1stprts> -DCOMPARE=$<TARGET_FILE:prts_golden_compare>
    -DREF=${CMAKE_SOURCE_DIR}/tests/golden/ref)
add_test(NAME render_golden
  COMMAND ${CMAKE_COMMAND} ${PRTS_GOLDEN_ARGS} -DSCENARIO=${CMAKE_SOURCE_DIR}/tests/golden/golden.lua
          -DOUT=${CMAKE_BINARY_DIR}/golden-out -DDIFF=${CMAKE_BINARY_DIR}/golden-diff
          -DUSER=${CMAKE_BINARY_DIR}/test-user/golden -P ${CMAKE_SOURCE_DIR}/tools/golden/run.cmake)
add_test(NAME render_golden_catches_fog_change
  COMMAND ${CMAKE_COMMAND} ${PRTS_GOLDEN_ARGS} -DSCENARIO=${CMAKE_SOURCE_DIR}/tests/golden/fog_changed.lua
          -DOUT=${CMAKE_BINARY_DIR}/golden-fog -DEXPECT=approach_retro.png
          -DUSER=${CMAKE_BINARY_DIR}/test-user/golden-fog -P ${CMAKE_SOURCE_DIR}/tools/golden/run.cmake)
set_tests_properties(render_golden render_golden_catches_fog_change PROPERTIES LABELS "gl")
