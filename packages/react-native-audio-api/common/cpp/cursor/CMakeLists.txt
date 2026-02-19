# internal cmake script for instructing how to build the audio api to generate compile_commands.json that cursor uses

cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(COMMON_CPP_DIR ${CMAKE_SOURCE_DIR}/..)

# Toggle: set to 1 to enable test mode, 0 for normal mode
set(RN_AUDIO_API_TEST 0)
set(REACT_NATIVE_DIR ../../../../../node_modules/react-native)


file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c" "${COMMON_CPP_DIR}/test/*.cpp")
list(REMOVE_ITEM COMMON_CPP_SOURCES
  "${COMMON_CPP_DIR}/audioapi/HostObjects/inputs/AudioRecorderHostObject.cpp"
)

set(INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include)
set(FFMPEG_INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/ffmpeg_include)
set(EXTERNAL_DIR ${COMMON_CPP_DIR}/audioapi/external)
set(JNI_LIBS_DIR ${COMMON_CPP_DIR}/../../android/src/main/jniLibs)

add_library(rnaudioapi_cursor STATIC ${COMMON_CPP_SOURCES} ${ANDROID_CPP_SOURCES})

foreach(lib IN ITEMS opus opusfile ogg vorbis vorbisenc vorbisfile crypto ssl)
  add_library(${lib} STATIC IMPORTED)
  set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${EXTERNAL_DIR}/${ANDROID_ABI}/lib${lib}.a)
endforeach()

foreach (lib IN ITEMS avcodec avformat avutil swresample)
  add_library(${lib} SHARED IMPORTED)
  set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/${ANDROID_ABI}/lib${lib}.so)
endforeach()

target_compile_definitions(rnaudioapi_cursor PRIVATE
  RN_AUDIO_API_TEST=${RN_AUDIO_API_TEST}
)

target_include_directories(
  rnaudioapi_cursor
  PRIVATE
  "${COMMON_CPP_DIR}"
  "${INCLUDE_DIR}"
  "${INCLUDE_DIR}/opus"
  "${FFMPEG_INCLUDE_DIR}"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/jsi"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${COMMON_CPP_DIR}/../../ios"
  "/opt/homebrew/Cellar/googletest/1.17.0/include"
)

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

target_include_directories(rnaudioapi_cursor PUBLIC ${INCLUDE_LIBRARIES})

target_link_libraries(rnaudioapi_cursor ${LINK_LIBRARIES})
