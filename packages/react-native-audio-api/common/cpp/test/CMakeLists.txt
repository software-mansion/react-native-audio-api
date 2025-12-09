cmake_minimum_required(VERSION 3.14)
project(rnaudioapi_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ROOT ${CMAKE_SOURCE_DIR}/../../../../..)
set(REACT_NATIVE_DIR "${ROOT}/node_modules/react-native")
set(JSI_DIR "${REACT_NATIVE_DIR}/ReactCommon/jsi")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/3983f67e32fb3e9294487b9d4f9586efa6e5d088.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(REACT_NATIVE_AUDIO_API_DIR "${ROOT}/node_modules/react-native-audio-api")

file(GLOB_RECURSE RNAUDIOAPI_SRC
  CONFIGURE_DEPENDS
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/*.cpp"
)

# exclude HostObjects from tests
list(FILTER RNAUDIOAPI_SRC EXCLUDE REGEX ".*/audioapi/HostObjects/.*\\.cpp$")
# exclude worklet nodes
list(FILTER RNAUDIOAPI_SRC EXCLUDE REGEX ".*/Worklet.*Node\\.cpp$")

list(REMOVE_ITEM RNAUDIOAPI_SRC
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/core/AudioContext.cpp"
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/libs/ffmpeg/FFmpegDecoding.cpp"
)

file(GLOB_RECURSE RNAUDIOAPI_LIBS
  CONFIGURE_DEPENDS
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/libs/*.c"
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/libs/*.h"
)

list(REMOVE_ITEM RNAUDIOAPI_LIBS
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.c"
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.c"
)

add_library(rnaudioapi STATIC ${RNAUDIOAPI_SRC})
add_library(rnaudioapi_libs STATIC ${RNAUDIOAPI_LIBS})

target_include_directories(rnaudioapi PUBLIC
  ${ROOT}/packages/react-native-audio-api/common/cpp
  ${JSI_DIR}
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  ${gtest_SOURCE_DIR}/include
  ${gmock_SOURCE_DIR}/include
)

target_include_directories(rnaudioapi_libs PUBLIC
  ${ROOT}/packages/react-native-audio-api/common/cpp
)

file(GLOB_RECURSE test_src
  CONFIGURE_DEPENDS
  "src/*.cpp"
)

add_executable(
  tests
  ${test_src}
)

add_compile_definitions(RN_AUDIO_API_ENABLE_WORKLETS=0)
add_compile_definitions(RN_AUDIO_API_TEST=1)
add_compile_definitions(RN_AUDIO_API_FFMPEG_DISABLED=1)

target_link_libraries(tests
  rnaudioapi
  rnaudioapi_libs
  GTest::gtest_main
  GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(tests)
