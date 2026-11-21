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

# JUnit XML reports for CI (one per test run).
set(PRTS_JUNIT_DIR ${CMAKE_BINARY_DIR}/junit)

prts_lua_cov_args(cov lua_unit_tests)
add_test(NAME lua_unit_tests COMMAND 1stprts --headless --test-lua --user ${CMAKE_BINARY_DIR}/test-user/lua ${cov}
  --junit ${PRTS_JUNIT_DIR}/lua_unit_tests.xml)
file(GLOB PRTS_SCENARIOS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/game/scenarios/*.lua)
foreach(scenario ${PRTS_SCENARIOS})
  get_filename_component(name ${scenario} NAME_WE)
  prts_lua_cov_args(cov scenario_${name})
  add_test(NAME scenario_${name}
    COMMAND 1stprts --headless --scenario ${scenario} --user ${CMAKE_BINARY_DIR}/test-user/${name} ${cov}
            --junit ${PRTS_JUNIT_DIR}/scenario_${name}.xml)
endforeach()

# Determinism checks (tests/game/determinism): record each walk, replay it (every tick's state
# hash must match the recording), record it again and require byte-identical recordings.
file(GLOB PRTS_DETERMINISM CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/game/determinism/*.lua)
set(PRTS_REPLAY_DIR ${CMAKE_BINARY_DIR}/replays)
foreach(walk ${PRTS_DETERMINISM})
  get_filename_component(name ${walk} NAME_WE)
  set(user ${CMAKE_BINARY_DIR}/test-user/determinism_${name})
  prts_lua_cov_args(cov determinism_${name}_record)
  add_test(NAME determinism_${name}_record
    COMMAND 1stprts --headless --scenario ${walk} --record ${PRTS_REPLAY_DIR}/${name}.rpl --user ${user} ${cov}
            --junit ${PRTS_JUNIT_DIR}/determinism_${name}_record.xml)
  prts_lua_cov_args(cov determinism_${name}_replay)
  add_test(NAME determinism_${name}_replay
    COMMAND 1stprts --headless --replay ${PRTS_REPLAY_DIR}/${name}.rpl --user ${user} ${cov}
            --junit ${PRTS_JUNIT_DIR}/determinism_${name}_replay.xml)
  add_test(NAME determinism_${name}_rerun
    COMMAND 1stprts --headless --scenario ${walk} --record ${PRTS_REPLAY_DIR}/${name}_again.rpl --user ${user})
  add_test(NAME determinism_${name}_identical
    COMMAND ${CMAKE_COMMAND} -E compare_files ${PRTS_REPLAY_DIR}/${name}.rpl ${PRTS_REPLAY_DIR}/${name}_again.rpl)
  set_tests_properties(determinism_${name}_record PROPERTIES FIXTURES_SETUP replay_${name})
  set_tests_properties(determinism_${name}_rerun PROPERTIES FIXTURES_SETUP replay_again_${name})
  set_tests_properties(determinism_${name}_replay PROPERTIES FIXTURES_REQUIRED replay_${name})
  set_tests_properties(determinism_${name}_identical PROPERTIES FIXTURES_REQUIRED "replay_${name};replay_again_${name}")
endforeach()
target_compile_definitions(prts_tests PRIVATE PRTS_TEST_TMP="${CMAKE_BINARY_DIR}/test-tmp")

# Rendered run of the tour (needs a display; CI runs ctest under xvfb-run with Mesa llvmpipe).
add_test(NAME render_tour
  COMMAND 1stprts --size 960x540 --scenario ${CMAKE_SOURCE_DIR}/tests/game/scenarios/tour.lua
          --out ${CMAKE_BINARY_DIR}/screenshots --user ${CMAKE_BINARY_DIR}/test-user/render)
set_tests_properties(render_tour PROPERTIES LABELS "gl")
target_include_directories(prts_tests PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)
