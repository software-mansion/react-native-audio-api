cmake_minimum_required(VERSION 3.12.0)
project(react-native-audio-api)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

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
  "../common/cpp/core/*.cpp"
  "../common/cpp/core/*.h"
  "../common/cpp/wrappers/*.cpp"
  "../common/cpp/wrappers/*.h"
  "../common/cpp/HostObjects/*.cpp"
  "../common/cpp/HostObjects/*.h"
  "../common/cpp/utils/*.cpp"
  "../common/cpp/utils/*.h"
  "../common/cpp/types/*.h"
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
  "${CMAKE_CURRENT_SOURCE_DIR}/libs/include"
)

set(fftw3 ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libfftw3.a)
set(libsndfile ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libsndfile.a)
set(libsamplerate ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libsamplerate.a)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

set(LINK_LIBRARIES
  ReactAndroid::jsi
  fbjni::fbjni
  android
  log
  oboe::oboe
  ${fftw3}
  ${libsndfile}
  ${libsamplerate}
)

if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 76)
  set(RN_VERSION_LINK_LIBRARIES
    ReactAndroid::reactnative
  )
else()
  set(RN_VERSION_LINK_LIBRARIES
    ReactAndroid::folly_runtime
    ReactAndroid::glog
    ReactAndroid::reactnativejni
  )
endif()

target_link_libraries(react-native-audio-api ${LINK_LIBRARIES} ${RN_VERSION_LINK_LIBRARIES})
