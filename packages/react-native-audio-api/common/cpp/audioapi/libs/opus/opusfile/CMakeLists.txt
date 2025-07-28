cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(OpusFilePackageVersion)
get_package_version(PACKAGE_VERSION PROJECT_VERSION)
string(REPLACE "." ";" PROJECT_VERSION_LIST ${PROJECT_VERSION})
list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)

project(OpusFile
  VERSION ${PROJECT_VERSION}
  LANGUAGES C
)

option(OP_DISABLE_HTTP "Disable HTTP support" OFF)
option(OP_DISABLE_FLOAT_API "Disable floating-point API" OFF)
option(OP_FIXED_POINT "Enable fixed-point calculation" OFF)
option(OP_ENABLE_ASSERTIONS "Enable assertions in code" OFF)
option(OP_DISABLE_EXAMPLES "Do not build example applications" OFF)
option(OP_DISABLE_DOCS "Do not build API documentation" OFF)

include(GNUInstallDirs)

find_package(Ogg REQUIRED)
find_package(Opus REQUIRED)

include(CMakePushCheckState)
include(CheckSymbolExists)
cmake_push_check_state(RESET)
include(CheckLibraryExists)
check_library_exists(m lrintf "" OP_HAVE_LIBM)
if(OP_HAVE_LIBM)
  list(APPEND CMAKE_REQUIRED_LIBRARIES "m")
endif()
check_symbol_exists(lrintf "math.h" OP_HAVE_LRINTF)
cmake_pop_check_state()

add_library(opusfile
  "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/info.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/opusfile.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/stream.c"
)
add_library(OpusFile::opusfile ALIAS opusfile)
set_target_properties(opusfile PROPERTIES
  PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(opusfile
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/opus>
)
target_link_libraries(opusfile
  PUBLIC
    Ogg::ogg
    Opus::opus
    $<$<BOOL:${OP_HAVE_LIBM}>:m>
)
target_compile_options(opusfile
  PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/wd4267>
    $<$<C_COMPILER_ID:MSVC>:/wd4244>
    $<$<C_COMPILER_ID:MSVC>:/wd4090>
    $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
    $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
    $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
)
target_compile_definitions(opusfile
  PRIVATE
    $<$<BOOL:${OP_DISABLE_FLOAT_API}>:OP_DISABLE_FLOAT_API>
    $<$<BOOL:${OP_FIXED_POINT}>:OP_FIXED_POINT>
    $<$<BOOL:${OP_ENABLE_ASSERTIONS}>:OP_ENABLE_ASSERTIONS>
    $<$<BOOL:${OP_HAVE_LRINTF}>:OP_HAVE_LRINTF>
)
install(TARGETS opusfile
  EXPORT OpusFileTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
)

if(NOT OP_DISABLE_HTTP)
  find_package(OpenSSL REQUIRED)

  include(CheckIncludeFile)
  include(CheckCSourceCompiles)
  cmake_push_check_state(RESET)
  if(NOT WIN32)
    check_include_file("sys/socket.h" OP_HAVE_SYS_SOCKET_H)
    if(NOT OP_HAVE_SYS_SOCKET_H)
      message(FATAL_ERROR "HTTP support requires a POSIX socket library")
    endif()
  endif()
  check_c_source_compiles(
    "#include <time.h>
    int main(void)
    {
      struct timespec ts;
      return clock_gettime(CLOCK_REALTIME, &ts);
    }"
    OP_HAVE_CLOCK_GETTIME
  )
  if(NOT OP_HAVE_CLOCK_GETTIME)
    check_symbol_exists(ftime "sys/timeb.h" OP_HAVE_FTIME)
    if(NOT OP_HAVE_FTIME)
      message(FATAL_ERROR "HTTP support requires either clock_gettime() or ftime()")
    endif()
  endif()
  cmake_pop_check_state()

  add_library(opusurl
    "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.h"
  )
  add_library(OpusFile::opusurl ALIAS opusurl)
  if(WIN32)
    target_sources(opusurl PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/wincerts.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/winerrno.h"
    )
  endif()
  set_target_properties(opusurl PROPERTIES
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  target_include_directories(opusurl
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/opus>
  )
  target_compile_definitions(opusurl
    PRIVATE
      $<$<BOOL:${OP_DISABLE_FLOAT_API}>:OP_DISABLE_FLOAT_API>
      $<$<BOOL:${OP_FIXED_POINT}>:OP_FIXED_POINT>
      $<$<BOOL:${OP_ENABLE_ASSERTIONS}>:OP_ENABLE_ASSERTIONS>
      $<$<BOOL:${OP_HAVE_LRINTF}>:OP_HAVE_LRINTF>
      $<$<BOOL:${OP_HAVE_CLOCK_GETTIME}>:OP_HAVE_CLOCK_GETTIME>
      $<$<BOOL:${OP_HAVE_FTIME}>:OP_HAVE_FTIME>
      OP_ENABLE_HTTP
  )
  target_link_libraries(opusurl
    PRIVATE
      opusfile
      OpenSSL::SSL
      $<$<C_COMPILER_ID:MSVC>:ws2_32>
      $<$<C_COMPILER_ID:MSVC>:crypt32>
      $<$<BOOL:${OP_HAVE_LIBM}>:m>
  )
  target_compile_options(opusurl
    PRIVATE
      $<$<C_COMPILER_ID:MSVC>:/wd4267>
      $<$<C_COMPILER_ID:MSVC>:/wd4244>
      $<$<C_COMPILER_ID:MSVC>:/wd4090>
      $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
      $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
      $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
  )
  install(TARGETS opusurl
    EXPORT OpusFileTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
  )
endif()

if(NOT OP_DISABLE_EXAMPLES)
  add_executable(opusfile_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/opusfile_example.c"
  )
  add_executable(OpusFile::opusfile_example ALIAS opusfile_example)
  if(WIN32)
    target_sources(opusfile_example PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/win32utf8.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/win32utf8.h"
    )
  endif()
  target_include_directories(opusfile_example
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/examples"
  )
  target_link_libraries(opusfile_example
    PRIVATE
      opusfile
      opusurl
  )
  target_compile_options(opusfile_example
    PRIVATE
      $<$<C_COMPILER_ID:MSVC>:/wd4267>
      $<$<C_COMPILER_ID:MSVC>:/wd4244>
      $<$<C_COMPILER_ID:MSVC>:/wd4090>
      $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
      $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
      $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
  )

  add_executable(seeking_example
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/seeking_example.c"
  )
  add_executable(OpusFile::seeking_example ALIAS seeking_example)
  if(WIN32)
    target_sources(seeking_example PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/win32utf8.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/win32utf8.h"
    )
  endif()
  target_include_directories(seeking_example
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/examples"
  )
  target_link_libraries(seeking_example
    PRIVATE
      opusfile
      opusurl
  )
  target_compile_options(seeking_example
    PRIVATE
      $<$<C_COMPILER_ID:MSVC>:/wd4267>
      $<$<C_COMPILER_ID:MSVC>:/wd4244>
      $<$<C_COMPILER_ID:MSVC>:/wd4090>
      $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
      $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
      $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
  )
endif()

if(NOT OP_DISABLE_DOCS)
  find_package(Doxygen OPTIONAL_COMPONENTS dot)

  set(DOXYGEN_PROJECT_BRIEF "Stand-alone decoder library for .opus files.")
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)

  set(DOXYGEN_QUIET YES)
  set(DOXYGEN_WARNINGS YES)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_WARN_IF_DOC_ERROR YES)
  set(DOXYGEN_WARN_NO_PARAMDOC YES)

  set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
  set(DOXYGEN_SORT_MEMBER_DOCS NO)

  set(DOXYGEN_PROJECT_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/doc/opus_logo.svg")

  set(DOXYGEN_FULL_PATH_NAMES NO)

  doxygen_add_docs(doxygen "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h" ALL USE_STAMP_FILE)
endif()

install(EXPORT OpusFileTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opusfile"
  NAMESPACE OpusFile::
)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpusFileConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/OpusFileConfig.cmake"
  INSTALL_DESTINATION
    "${CMAKE_INSTALL_LIBDIR}/cmake/opusfile"
)
write_basic_package_version_file(
  "OpusFileConfigVersion.cmake"
  VERSION "${PACKAGE_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OpusFileConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpusFileConfigVersion.cmake"
  DESTINATION
    "${CMAKE_INSTALL_LIBDIR}/cmake/opusfile"
)
