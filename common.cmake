include_guard()

# Utils

include(${CMAKE_CURRENT_LIST_DIR}/utils/fetchContentFromDeps.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/utils/rpath.cmake)

# Defs

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GCC TRUE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CLANG TRUE)
endif()

include(CheckLinkerFlag)

# Enable a linker option if supported
macro(try_add_link_option name option)
  check_linker_flag(CXX "${option}" linker_has_${name})
  if(linker_has_${name})
    add_link_options("${option}")
  endif()
endmacro()

# Misc

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to not building tests
option(BUILD_TESTING "" OFF)

# Compiler options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# For all built shared-objects, request that exports are explicitly defined
# Note, this isn't necessarily enough - see also version-script option
if(UNIX)
  add_compile_options(-fvisibility=hidden)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(LINUX)
  add_link_options(LINKER:-z,noexecstack)
  add_link_options(LINKER:-z,relro,-z,now)
  try_add_link_option(enable-new-dtags LINKER:--enable-new-dtags)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  # Building for minimized footprint
  # Turn on additional footprint optimizations
  # (CMake already does -Os)

  # Interprocedural optimization
  cmake_policy(SET CMP0069 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

  # Dead code elimination - limited support
  if(LINUX)
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options("LINKER:--gc-sections,-Map=link.map")
  endif()
endif()

if(MSVC)
  if(MSVC_VERSION LESS 1910)
    message(SEND_ERROR "Microsoft Visual Studio 2019 with support for C++17 \
std::variant is required")
  endif()
  if(MSVC_TOOLSET_VERSION LESS 141)
    message(SEND_ERROR "MSVC Toolset v141 is required for std::variant support")
  endif()
  add_compile_options("/EHsc")
endif()

# Debugger options

if(LINUX)
  add_compile_options($<$<CONFIG:Debug>:-ggdb3>)
  if(GCC)
    add_compile_options($<$<CONFIG:Debug>:-Og>)
  endif()
endif()

if(APPLE)
  add_compile_options($<$<CONFIG:Debug>:-g3>)
  if(CLANG)
    add_compile_options($<$<CONFIG:Debug>:-glldb>)
  endif()
endif()

# Sanitizers

# Enable a sanitizer
macro(enable_debug_sanitizer name)
  add_compile_options($<$<CONFIG:Debug>:-fsanitize=${name}>)
  add_link_options($<$<CONFIG:Debug>:-fsanitize=${name}>)
endmacro()

if(NOT WIN32)
  if(GCC)
    enable_debug_sanitizer(undefined)
  endif()

  # Thread and address sanitizer are mutually exclusive
  #enable_debug_sanitizer(address)
  #enable_debug_sanitizer(thread)
endif()
