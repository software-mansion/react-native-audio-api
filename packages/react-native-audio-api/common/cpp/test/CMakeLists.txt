cmake_minimum_required(VERSION 3.14)
project(rnaudioapi_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ROOT ${CMAKE_SOURCE_DIR}/../../../../..)
set(REACT_NATIVE_DIR "${ROOT}/node_modules/react-native")
set(JSI_DIR "${REACT_NATIVE_DIR}/ReactCommon/jsi")

include(CMakePrintHelpers)
cmake_print_variables(ROOT)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/c67de117379f4d1c889c7581a0a76aa0979c2083.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER}")

enable_testing()
add_compile_definitions(TESTING)

file(GLOB_RECURSE RNAUDIOAPI_SRC
  CONFIGURE_DEPENDS
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/AudioNode.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/AudioParam.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/BaseAudioContext.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/OfflineAudioContext.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/analysis/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/destinations/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/effects/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/inputs/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/sources/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/types/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/core/utils/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/dsp/*.cpp"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/utils/*.cpp"
)

file(GLOB_RECURSE RNAUDIOAPI_LIBS
  CONFIGURE_DEPENDS
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/libs/base64/*.h"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/libs/miniaudio/*.h"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/libs/pffft/*.c"
  "${ROOT}/packages/react-native-audio-api/common/cpp/audioapi/libs/signalsmith-stretch/*.h"
)

add_library(rnaudioapi STATIC ${RNAUDIOAPI_SRC})
add_library(rnaudioapi_libs STATIC ${RNAUDIOAPI_LIBS})

target_include_directories(rnaudioapi PUBLIC
  ${ROOT}/packages/react-native-audio-api/common/cpp
  ${JSI_DIR}
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
)

target_include_directories(rnaudioapi_libs PUBLIC
  ${ROOT}/packages/react-native-audio-api/common/cpp
)

add_executable(
  tests
  OscillatorTest.cpp
)

target_link_libraries(tests
  rnaudioapi
  rnaudioapi_libs
  GTest::gtest_main
  GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(tests)