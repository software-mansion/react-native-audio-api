cmake_minimum_required(VERSION 3.9.0)
project(react-native-audio-context)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)

include(../node_modules/react-native/ReactAndroid/cmake-utils/folly-flags.cmake)
add_compile_options(${folly_FLAGS})

file(GLOB_RECURSE SOURCE_FILES
  "src/main/cpp/*.cpp"
  "src/main/cpp/*.h"
  "../cpp/*.cpp"
  "../cpp/*.h"
)

add_library(react-native-audio-context SHARED ${SOURCE_FILES})

file(GLOB DIRS CONFIGURE_DEPENDS "src/main/cpp/**" "../cpp/**")

target_include_directories(
  react-native-audio-context
  PRIVATE
  ${DIRS}
  ../node_modules/react-native/ReactCommon/jsi
  ../node_modules/react-native/ReactAndroid/src/main/jni/react/jni
  ../node_modules/react-native/ReactAndroid/src/main/jni/third-party/folly
)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

set(LINK_LIBRARIES
  ReactAndroid::jsi
  ReactAndroid::reactnativejni
  fbjni::fbjni
  ReactAndroid::folly_runtime
  android
  log
)

target_link_libraries(react-native-audio-context ${LINK_LIBRARIES})
