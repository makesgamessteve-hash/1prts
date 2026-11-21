# The 1stPRTS executable: a thin launcher around libprts.
add_executable(1stprts main.c)
target_link_libraries(1stprts PRIVATE prts)
prts_set_warnings(1stprts)
prts_set_coverage(1stprts)
# Put the data folder next to the binary on every build, so the build output runs as is.
add_custom_target(prts_data ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
  COMMENT "Copying data/ next to the executable")
add_dependencies(1stprts prts_data)
