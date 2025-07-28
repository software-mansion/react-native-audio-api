cmake_minimum_required(VERSION 3.12.0)

set(ANDROID_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(OGG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../common/cpp/audioapi/libs/opus/ogg)
set(OPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../common/cpp/audioapi/libs/opus/opus)
set(OPUSFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../common/cpp/audioapi/libs/opus/opusfile)

file(GLOB_RECURSE ANDROID_CPP_SOURCES CONFIGURE_DEPENDS "${ANDROID_CPP_DIR}/audioapi/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")

file(GLOB_RECURSE OGG_SRC "${OGG_DIR}/src/*.c")
file(GLOB_RECURSE OPUS_SRC "${OPUS_DIR}/src/*.c")
file(GLOB_RECURSE OPUSFILE_SRC "${OPUSFILE_DIR}/src/*.c")

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})
add_library(ogg STATIC ${OGG_SRC})
add_library(opus STATIC ${OPUS_SRC})
add_library(opusfile STATIC ${OPUSFILE_SRC})

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
