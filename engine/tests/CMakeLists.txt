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
  # A scenario can ask for extra options with a line "-- ctest-args: --front-end --mod x". Test
  # mods (tests/game/mods) are installed into every scenario's user area.
  file(STRINGS ${scenario} extra REGEX "^-- ctest-args: " LIMIT_COUNT 1)
  string(REPLACE "-- ctest-args: " "" extra "${extra}")
  separate_arguments(extra)
  file(COPY ${CMAKE_SOURCE_DIR}/tests/game/mods DESTINATION ${CMAKE_BINARY_DIR}/test-user/${name})
  add_test(NAME scenario_${name}
    COMMAND 1stprts --headless --scenario ${scenario} --user ${CMAKE_BINARY_DIR}/test-user/${name} ${extra} ${cov}
            --junit ${PRTS_JUNIT_DIR}/scenario_${name}.xml)
  # "-- ctest-labels: smoke" puts a scenario into the commander-is-there smoke set (walk, stairs,
  # door, lift, war table; `ctest -L smoke`), which web/tests/smoke.test.js also runs in the browser.
  file(STRINGS ${scenario} labels REGEX "^-- ctest-labels: " LIMIT_COUNT 1)
  if(labels)
    string(REPLACE "-- ctest-labels: " "" labels "${labels}")
    separate_arguments(labels)
    set_tests_properties(scenario_${name} PROPERTIES LABELS "${labels}")
  endif()
endforeach()

# The fun audit's survey (tests/fun/survey.lua, spec: Ways through a map stay open): its oracle
# walks every way of Harrowmark's route graph there and back with the audit's mod (tests/fun/mods,
# never shipped). The full audit is tools/fun/audit.py (tools/fun/README.md).
file(COPY ${CMAKE_SOURCE_DIR}/tests/fun/mods DESTINATION ${CMAKE_BINARY_DIR}/test-user/fun_survey)
add_test(NAME fun_survey
  COMMAND 1stprts --headless --scenario ${CMAKE_SOURCE_DIR}/tests/fun/survey.lua --user ${CMAKE_BINARY_DIR}/test-user/fun_survey
          --mod fun_eval --junit ${PRTS_JUNIT_DIR}/fun_survey.xml)
set_tests_properties(fun_survey PROPERTIES LABELS "fun")

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

# Render golden images (spec: Visual regression): render fixed views and compare them with
# tests/golden/ref (8 levels per channel, at most 0.5% of pixels). Differences go to
# golden-diff/. A second test proves that a changed fog color is caught.
add_executable(prts_golden_compare ${CMAKE_SOURCE_DIR}/tools/golden/compare.c)
target_link_libraries(prts_golden_compare PRIVATE prts)
target_include_directories(prts_golden_compare PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)
prts_set_warnings(prts_golden_compare)
prts_set_coverage(prts_golden_compare) # links the coverage runtime libprts needs in coverage builds
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
