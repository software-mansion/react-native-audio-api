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

file(GLOB_RECURSE RNAUDIOAPI_SRC
  CONFIGURE_DEPENDS
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/*.cpp"
  "${ROOT}/node_modules/react-native-audio-api/android/src/main/cpp/audioapi/android/core/utils/AudioDecoder.cpp"
)

# exclude HostObjects from tests
list(FILTER RNAUDIOAPI_SRC EXCLUDE REGEX ".*/audioapi/HostObjects/.*\\.cpp$")

list(REMOVE_ITEM RNAUDIOAPI_SRC
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/AudioContext.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/effects/WorkletNode.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/effects/WorkletProcessingNode.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/sources/WorkletSourceNode.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/sources/StreamerNode.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/core/sources/StreamerNode.h"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/FFmpegDecoding.cpp"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/FFmpegDecoding.h"
)

file(GLOB_RECURSE RNAUDIOAPI_LIBS
  CONFIGURE_DEPENDS
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/*.c"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/*.h"
)

list(REMOVE_ITEM RNAUDIOAPI_LIBS
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.c"
  "${ROOT}/node_modules/react-native-audio-api/common/cpp/audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.c"
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

add_compile_definitions(AUDIO_API_TEST_SUITE)
add_compile_definitions(RN_AUDIO_API_ENABLE_WORKLETS=0)
add_compile_definitions(RN_AUDIO_API_TEST=1)

target_link_libraries(tests
  rnaudioapi
  rnaudioapi_libs
  GTest::gtest_main
  GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(tests)
