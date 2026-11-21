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
  # A scenario can ask for extra options with a line "-- ctest-args: --front-end --mod x". Test
  # mods (tests/game/mods) are installed into every scenario's user area.
  file(STRINGS ${scenario} extra REGEX "^-- ctest-args: " LIMIT_COUNT 1)
  string(REPLACE "-- ctest-args: " "" extra "${extra}")
  separate_arguments(extra)
  file(COPY ${CMAKE_SOURCE_DIR}/tests/game/mods DESTINATION ${CMAKE_BINARY_DIR}/test-user/${name})
  add_test(NAME scenario_${name}
    COMMAND 1stprts --headless --scenario ${scenario} --user ${CMAKE_BINARY_DIR}/test-user/${name} ${extra} ${cov})
endforeach()
target_compile_definitions(prts_tests PRIVATE PRTS_TEST_TMP="${CMAKE_BINARY_DIR}/test-tmp")

# Rendered run of the tour (needs a display; CI runs ctest under xvfb-run with Mesa llvmpipe).
add_test(NAME render_tour
  COMMAND 1stprts --size 960x540 --scenario ${CMAKE_SOURCE_DIR}/tests/game/scenarios/tour.lua
          --out ${CMAKE_BINARY_DIR}/screenshots --user ${CMAKE_BINARY_DIR}/test-user/render)
set_tests_properties(render_tour PROPERTIES LABELS "gl")
target_include_directories(prts_tests PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)
