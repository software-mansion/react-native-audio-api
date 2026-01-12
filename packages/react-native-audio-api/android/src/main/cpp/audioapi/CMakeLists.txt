cmake_minimum_required(VERSION 3.12.0)

file(GLOB_RECURSE ANDROID_CPP_SOURCES CONFIGURE_DEPENDS "${ANDROID_CPP_DIR}/audioapi/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")

if (RN_AUDIO_API_FFMPEG_DISABLED)
  list(REMOVE_ITEM COMMON_CPP_SOURCES
    "${COMMON_CPP_DIR}/audioapi/libs/ffmpeg/FFmpegDecoding.cpp"
  )
endif()

set_source_files_properties(
  ${COMMON_CPP_SOURCES}/dsp/*.cpp
  PROPERTIES
  COMPILE_FLAGS "-O3"
)

set(INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include)
set(FFMPEG_INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/ffmpeg_include)
set(EXTERNAL_DIR ${COMMON_CPP_DIR}/audioapi/external)
set(JNI_LIBS_DIR ${COMMON_CPP_DIR}/../../android/src/main/jniLibs)

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})

foreach(lib IN ITEMS opus opusfile ogg vorbis vorbisenc vorbisfile crypto ssl)
  add_library(${lib} STATIC IMPORTED)
  set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${EXTERNAL_DIR}/${ANDROID_ABI}/lib${lib}.a)
endforeach()

if (NOT RN_AUDIO_API_FFMPEG_DISABLED)
  foreach (lib IN ITEMS avcodec avformat avutil swresample)
    add_library(${lib} SHARED IMPORTED)
    set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/${ANDROID_ABI}/lib${lib}.so)
  endforeach()
endif()

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  set(BUILD_TYPE "debug")
else()
  set(BUILD_TYPE "release")
endif()

target_include_directories(
  react-native-audio-api
  PRIVATE
  "${COMMON_CPP_DIR}"
  "${ANDROID_CPP_DIR}"
  "${INCLUDE_DIR}"
  "${INCLUDE_DIR}/opus"
  "${FFMPEG_INCLUDE_DIR}"
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

set(INCLUDE_LIBRARIES
  "${COMMON_CPP_DIR}"
  "${ANDROID_CPP_DIR}"
  "${INCLUDE_DIR}"
  "${INCLUDE_DIR}/opus"
  "${INCLUDE_DIR}/vorbis"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
)

if(RN_AUDIO_API_WORKLETS_ENABLED)
  # Import the worklets library (similar to how Reanimated does it)
  add_library(worklets SHARED IMPORTED)
  set_target_properties(
    worklets
    PROPERTIES
      IMPORTED_LOCATION
      "${REACT_NATIVE_WORKLETS_DIR}/build/intermediates/cmake/${BUILD_TYPE}/obj/${ANDROID_ABI}/libworklets.so"
  )
  list(APPEND INCLUDE_LIBRARIES
    "${REACT_NATIVE_WORKLETS_DIR}/../Common/cpp"
    "${REACT_NATIVE_WORKLETS_DIR}/src/main/cpp"
  )
  list(APPEND LINK_LIBRARIES worklets)
endif()

target_include_directories(
  react-native-audio-api
  PRIVATE
  ${INCLUDE_LIBRARIES}
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

set(RN_VERSION_LINK_LIBRARIES
  ReactAndroid::reactnative
)

if(RN_AUDIO_API_WORKLETS_ENABLED)
  target_compile_definitions(
    react-native-audio-api
    PRIVATE RN_AUDIO_API_ENABLE_WORKLETS=1
  )
else()
  target_compile_definitions(
    react-native-audio-api
    PRIVATE RN_AUDIO_API_ENABLE_WORKLETS=0
  )
endif()

if (RN_AUDIO_API_FFMPEG_DISABLED)
  target_compile_definitions(
    react-native-audio-api
    PRIVATE RN_AUDIO_API_FFMPEG_DISABLED=1
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
  crypto
  ssl
  z
)

if(NOT RN_AUDIO_API_FFMPEG_DISABLED)
  target_link_libraries(react-native-audio-api
    avcodec
    avformat
    avutil
    swresample
  )
endif()
