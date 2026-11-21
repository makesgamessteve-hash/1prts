# Strict warnings for 1stPRTS's own code (not third-party code).
function(prts_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /wd4100 /wd4996)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion
      -Wstrict-prototypes -Wmissing-prototypes -Werror)
  endif()
endfunction()

# Coverage instrumentation for 1stPRTS's own code.
function(prts_set_coverage target)
  if(PRTS_COVERAGE AND NOT MSVC)
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PRIVATE --coverage)
  endif()
endfunction()
