# Third-party libraries. Built without project warnings and without coverage.
file(GLOB PRTS_LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/*.c)
add_library(prts_lua STATIC ${PRTS_LUA_SOURCES})
target_include_directories(prts_lua SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lua/src)
set_target_properties(prts_lua PROPERTIES C_STANDARD 11 C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON)
if(UNIX AND NOT EMSCRIPTEN)
  target_compile_definitions(prts_lua PRIVATE LUA_USE_POSIX)
  target_link_libraries(prts_lua PUBLIC m)
endif()
if(MSVC)
  target_compile_options(prts_lua PRIVATE /W0)
else()
  target_compile_options(prts_lua PRIVATE -w)
endif()

add_library(prts_stb INTERFACE)
target_include_directories(prts_stb SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

add_library(prts_utest INTERFACE)
target_include_directories(prts_utest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/utest)

add_library(prts_microui STATIC ${CMAKE_CURRENT_SOURCE_DIR}/microui/microui.c)
target_include_directories(prts_microui SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/microui)
set_target_properties(prts_microui PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(prts_microui PRIVATE /W0)
else()
  # microui packs its draw commands without padding; that is fine on every target we
  # build for, so the sanitizer's alignment check is left out for it (engine code reads the
  # commands through aligned copies).
  target_compile_options(prts_microui PRIVATE -w -fno-sanitize=alignment)
endif()
