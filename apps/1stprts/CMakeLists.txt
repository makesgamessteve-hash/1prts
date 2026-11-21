# The 1stPRTS executable: a thin launcher around libprts.
add_executable(1stprts main.c)
target_link_libraries(1stprts PRIVATE prts)
prts_set_warnings(1stprts)
prts_set_coverage(1stprts)
if(EMSCRIPTEN)
  # 1stprts.html/.js/.wasm, with data/ packed into 1stprts.data next to them.
  include(PrtsWeb)
  prts_web_executable(1stprts)
  return()
endif()
# Put the data folder next to the binary on every build, so the build output runs as is.
add_custom_target(prts_data ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
  COMMENT "Copying data/ next to the executable")
add_dependencies(1stprts prts_data)

# --- release package (task 14.1): the executable with data/ next to it, the licenses of the
# code and the art, and every third-party license. `cpack` in the build folder makes the archive.
install(TARGETS 1stprts RUNTIME DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/ DESTINATION data)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_SOURCE_DIR}/LICENSE-ASSETS ${CMAKE_SOURCE_DIR}/LICENSING.md
              ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/third_party/README.md DESTINATION licenses)
file(GLOB PRTS_THIRD_PARTY_DIRS LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/third_party/*)
foreach(dir ${PRTS_THIRD_PARTY_DIRS})
  if(IS_DIRECTORY ${dir})
    get_filename_component(lib ${dir} NAME)
    file(GLOB licenses ${dir}/LICENSE* ${dir}/COPYING*)
    install(FILES ${licenses} DESTINATION licenses/${lib})
  endif()
endforeach()
if(DEFINED sdl3_SOURCE_DIR AND EXISTS ${sdl3_SOURCE_DIR}/LICENSE.txt)
  install(FILES ${sdl3_SOURCE_DIR}/LICENSE.txt DESTINATION licenses/SDL3 RENAME LICENSE)
endif()

if(WIN32)
  set(PRTS_PACKAGE_PLATFORM windows)
  set(CPACK_GENERATOR ZIP)
elseif(APPLE)
  set(PRTS_PACKAGE_PLATFORM macos)
  set(CPACK_GENERATOR ZIP)
else()
  set(PRTS_PACKAGE_PLATFORM linux)
  set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_PACKAGE_NAME 1stprts)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME 1stprts-${PROJECT_VERSION}-${PRTS_PACKAGE_PLATFORM})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
include(CPack)
