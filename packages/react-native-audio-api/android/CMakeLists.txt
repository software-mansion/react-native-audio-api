cmake_minimum_required(VERSION 3.9.0)
project(react-native-audio-api)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

# Detect the operating system
if(APPLE)
    set(HAVE_ACCELERATE TRUE)
endif()

# Detect the processor and SIMD support
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(HAVE_ARM_NEON_INTRINSICS TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(HAVE_X86_SSE2 TRUE)
endif()

include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

file(GLOB_RECURSE SOURCE_FILES
  "src/main/cpp/*.cpp"
  "src/main/cpp/*.h"
  "../common/cpp/*.cpp"
  "../common/cpp/*.h"
)

add_library(react-native-audio-api SHARED ${SOURCE_FILES})

file(GLOB DIRS CONFIGURE_DEPENDS "src/main/cpp/**" "../common/cpp/**")

target_include_directories(
  react-native-audio-api
  PRIVATE
  ${DIRS}
  "${REACT_NATIVE_DIR}/ReactCommon/jsi"
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/jni"
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/third-party/folly"
)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
find_package (oboe REQUIRED CONFIG)

set(LINK_LIBRARIES
  ReactAndroid::jsi
  ReactAndroid::reactnativejni
  fbjni::fbjni
  ReactAndroid::folly_runtime
  android
  log
  oboe::oboe
)

target_link_libraries(react-native-audio-api ${LINK_LIBRARIES})
