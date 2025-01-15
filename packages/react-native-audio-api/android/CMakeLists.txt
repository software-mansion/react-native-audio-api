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

file(GLOB_RECURSE ANDROID_CPP_SOURCES CONFIGURE_DEPENDS "src/main/cpp/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "../common/cpp/*.cpp")

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})

file(GLOB ANDROID_CPP_DIR CONFIGURE_DEPENDS "src/main/cpp/**")
file(GLOB COMMON_CPP_DIR CONFIGURE_DEPENDS "../common/cpp/**")

target_include_directories(
  react-native-audio-api
  PUBLIC
  ${ANDROID_CPP_DIR}
  ${COMMON_CPP_DIR}
)

target_include_directories(
  react-native-audio-api
  PRIVATE
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule"
  "${REACT_NATIVE_DIR}/ReactCommon/react/nativemodule/core/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs/include"
)

set(fftw3 ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libfftw3.a)

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
)

if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 76)
  set(RN_VERSION_LINK_LIBRARIES
    ReactAndroid::reactnative
  )
else()
  set(RN_VERSION_LINK_LIBRARIES
    ReactAndroid::folly_runtime
    ReactAndroid::react_nativemodule_core
    ReactAndroid::glog
    ReactAndroid::reactnativejni
  )
endif()

target_link_libraries(react-native-audio-api ${LINK_LIBRARIES} ${RN_VERSION_LINK_LIBRARIES})
