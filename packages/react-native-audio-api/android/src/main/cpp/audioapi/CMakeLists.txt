cmake_minimum_required(VERSION 3.12.0)

file(GLOB_RECURSE ANDROID_CPP_SOURCES CONFIGURE_DEPENDS "${ANDROID_CPP_DIR}/audioapi/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")

set(OPUS_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/opus/build-android)
set(OPUSFILE_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/opusfile/build-android)
set(OGG_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/ogg/build-android)

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})

link_directories(
  ${OPUS_BUILD_DIR}
  ${OPUSFILE_BUILD_DIR}
  ${OGG_BUILD_DIR}
)

add_library(opus STATIC IMPORTED)
set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${OPUS_BUILD_DIR}/libopus.a)

add_library(opusfile STATIC IMPORTED)
set_target_properties(opusfile PROPERTIES IMPORTED_LOCATION ${OPUSFILE_BUILD_DIR}/libopusfile.a)

add_library(ogg STATIC IMPORTED)
set_target_properties(ogg PROPERTIES IMPORTED_LOCATION ${OGG_BUILD_DIR}/libogg.a)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

target_include_directories(
  react-native-audio-api
  PRIVATE
  "${COMMON_CPP_DIR}"
  "${ANDROID_CPP_DIR}"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/ogg/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/ogg/build-android/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../opus/opus/include"
)

set(LINK_LIBRARIES
  ReactAndroid::jsi
  fbjni::fbjni
  android
  log
  oboe::oboe
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

target_link_libraries(react-native-audio-api
  ${LINK_LIBRARIES}
  ${RN_VERSION_LINK_LIBRARIES}
  opusfile
  opus
  ogg
)
