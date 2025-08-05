cmake_minimum_required(VERSION 3.12.0)

file(GLOB_RECURSE ANDROID_CPP_SOURCES CONFIGURE_DEPENDS "${ANDROID_CPP_DIR}/audioapi/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")

set(EXTERNAL_DIR ${COMMON_CPP_DIR}/audioapi/external)
set(ARCH_LIB_DIR ${EXTERNAL_DIR}/${ANDROID_ABI})
set(INCLUDE_DIR ${EXTERNAL_DIR}/include)

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})

foreach(lib IN ITEMS opus opusfile ogg vorbis vorbisenc vorbisfile)
  add_library(${lib} STATIC IMPORTED)
  set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${ARCH_LIB_DIR}/lib${lib}.a)
endforeach()

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

target_include_directories(
  react-native-audio-api
  PRIVATE
  "${COMMON_CPP_DIR}"
  "${ANDROID_CPP_DIR}"
  "${INCLUDE_DIR}"
  "${INCLUDE_DIR}/opus"
  "${INCLUDE_DIR}/vorbis"
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
  vorbis
  vorbisenc
  vorbisfile
)
