cmake_minimum_required(VERSION 3.13)
project(react-native-audio-worklets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB_RECURSE AUDIO_WORKLETS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/common/cpp/audioworklets/*.cpp")

add_library(react-native-audio-worklets STATIC ${AUDIO_WORKLETS_SOURCES})

target_compile_definitions(react-native-audio-worklets PRIVATE RCT_NEW_ARCH_ENABLED)

target_include_directories(react-native-audio-worklets
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/common/cpp"
)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

find_package(react-native-worklets REQUIRED CONFIG)
find_package(react-native-audio-api REQUIRED CONFIG)

target_link_libraries(react-native-audio-worklets
  fbjni::fbjni
  ReactAndroid::jsi
  ReactAndroid::reactnative
  react_codegen_rnaudioworklets
  react-native-worklets::worklets
  react-native-audio-api::react-native-audio-api
  android
  log
)
