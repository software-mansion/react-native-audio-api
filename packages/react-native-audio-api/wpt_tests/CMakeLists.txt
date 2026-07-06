cmake_minimum_required(VERSION 3.14)
project(rnaudioapi_node_bindings)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PACKAGE_ROOT ${CMAKE_SOURCE_DIR}/..)
set(ROOT ${PACKAGE_ROOT}/../..)
set(REACT_NATIVE_DIR "${ROOT}/node_modules/react-native")
set(JSI_DIR "${REACT_NATIVE_DIR}/ReactCommon/jsi")
set(JSI_IMPL_DIR "${JSI_DIR}/jsi")
set(FFMPEG_INCLUDE_DIR "${PACKAGE_ROOT}/common/cpp/audioapi/external/include_ffmpeg")
include(FetchContent)

# Upstream has no release tags; pin to a known commit. GCC on Linux requires
# explicit <atomic>/<utility> includes that are missing from upstream sources.
set(NODE_API_JSI_GIT_TAG 4b8a4a59c4e1b7b988b809dedffd89a7662aa785)

FetchContent_Declare(
  node_api_jsi
  GIT_REPOSITORY https://github.com/microsoft/node-api-jsi.git
  GIT_TAG ${NODE_API_JSI_GIT_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(node_api_jsi)
if(NOT node_api_jsi_POPULATED)
  FetchContent_Populate(node_api_jsi)
endif()

set(NODE_API_JSI_RUNTIME_SRC "${node_api_jsi_SOURCE_DIR}/src/NodeApiJsiRuntime.cpp")
set(NODE_API_JSI_RUNTIME_PATCHED_DIR "${CMAKE_BINARY_DIR}/generated/node_api_jsi")
set(NODE_API_JSI_RUNTIME_PATCHED_SRC "${NODE_API_JSI_RUNTIME_PATCHED_DIR}/NodeApiJsiRuntime.cpp")
file(MAKE_DIRECTORY "${NODE_API_JSI_RUNTIME_PATCHED_DIR}")
file(READ "${NODE_API_JSI_RUNTIME_SRC}" NODE_API_JSI_RUNTIME_CONTENT)
string(REPLACE
  "#include \"NodeApiJsiRuntime.h\"\n\n#include <algorithm>"
  "#include \"NodeApiJsiRuntime.h\"\n\n#include <atomic>\n#include <utility>\n#include <algorithm>"
  NODE_API_JSI_RUNTIME_CONTENT
  "${NODE_API_JSI_RUNTIME_CONTENT}")
set(NODE_API_JSI_SET_TRAP_ORIGINAL
"  runInMethodContext(
      \"HostObject::set\", [&hostObject, &propertyId, &value, this]() {
        hostObject->set(*this, propertyId, value);
      });
  return getUndefined();")
set(NODE_API_JSI_SET_TRAP_PATCHED
"  runInMethodContext(
      \"HostObject::set\", [&hostObject, &propertyId, &value, this]() {
        hostObject->set(*this, propertyId, value);
      });
  return getBoolean(true);")
string(REPLACE
  "${NODE_API_JSI_SET_TRAP_ORIGINAL}"
  "${NODE_API_JSI_SET_TRAP_PATCHED}"
  NODE_API_JSI_RUNTIME_CONTENT
  "${NODE_API_JSI_RUNTIME_CONTENT}")
file(WRITE "${NODE_API_JSI_RUNTIME_PATCHED_SRC}" "${NODE_API_JSI_RUNTIME_CONTENT}")

file(GLOB_RECURSE RNAUDIOAPI_COMMON_SOURCES
  CONFIGURE_DEPENDS
  "${PACKAGE_ROOT}/common/cpp/audioapi/*.cpp"
)

list(FILTER RNAUDIOAPI_COMMON_SOURCES EXCLUDE REGEX ".*/android/.*\\.cpp$")
list(FILTER RNAUDIOAPI_COMMON_SOURCES EXCLUDE REGEX ".*/ios/.*\\.cpp$")
list(FILTER RNAUDIOAPI_COMMON_SOURCES EXCLUDE REGEX ".*/ios/.*\\.mm$")
list(FILTER RNAUDIOAPI_COMMON_SOURCES EXCLUDE REGEX ".*/HostObjects/inputs/AudioRecorderHostObject\\.cpp$")

list(APPEND RNAUDIOAPI_COMMON_SOURCES
  "${CMAKE_SOURCE_DIR}/src/addon_init.cpp"
  "${CMAKE_SOURCE_DIR}/src/NodeJSRuntimeApi.cpp"
  "${CMAKE_SOURCE_DIR}/src/jsi_install.cpp"
  "${CMAKE_SOURCE_DIR}/src/SyncCallInvoker.cpp"
  "${CMAKE_SOURCE_DIR}/src/NodeAudioPlayer.cpp"
  "${NODE_API_JSI_RUNTIME_PATCHED_SRC}"
  "${node_api_jsi_SOURCE_DIR}/src/ApiLoaders/NodeApi.cpp"
  "${node_api_jsi_SOURCE_DIR}/src/ApiLoaders/JSRuntimeApi.cpp"
  "${JSI_IMPL_DIR}/jsi.cpp"
  "${JSI_IMPL_DIR}/jsilib-posix.cpp"
)

file(GLOB_RECURSE RNAUDIOAPI_LIB_SOURCES
  CONFIGURE_DEPENDS
  "${PACKAGE_ROOT}/common/cpp/audioapi/libs/*.c"
)

add_library(${PROJECT_NAME} SHARED
  ${RNAUDIOAPI_COMMON_SOURCES}
  ${RNAUDIOAPI_LIB_SOURCES}
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
  PREFIX ""
  SUFFIX ".node"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  JSI_VERSION=21
  RN_AUDIO_API_NODE=1
  RN_AUDIO_API_ENABLE_WORKLETS=0
  RN_AUDIO_API_FFMPEG_DISABLED=1
  RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED=1
  MA_NO_LIBOPUS=1
  MA_NO_LIBVORBIS=1
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${PACKAGE_ROOT}/common/cpp
  ${FFMPEG_INCLUDE_DIR}
  "${FFMPEG_INCLUDE_DIR}/libavcodec"
  "${FFMPEG_INCLUDE_DIR}/libavformat"
  "${FFMPEG_INCLUDE_DIR}/libavutil"
  "${FFMPEG_INCLUDE_DIR}/libswresample"
  "${node_api_jsi_SOURCE_DIR}/src"
  "${node_api_jsi_SOURCE_DIR}/node-api"
  ${CMAKE_SOURCE_DIR}/src
  ${JSI_DIR}
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  ${CMAKE_JS_INC}
)

target_link_libraries(${PROJECT_NAME}
  ${CMAKE_JS_LIB}
)
