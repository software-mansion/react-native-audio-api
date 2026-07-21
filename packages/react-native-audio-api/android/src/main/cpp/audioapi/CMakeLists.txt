cmake_minimum_required(VERSION 3.12.0)

file(GLOB_RECURSE ANDROID_CPP_SOURCES_ABS CONFIGURE_DEPENDS "${ANDROID_CPP_DIR}/audioapi/*.cpp")
file(GLOB_RECURSE COMMON_CPP_SOURCES_ABS CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")
file(GLOB_RECURSE DSP_CPP_SOURCES_ABS CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/dsp/*.cpp")

foreach(src IN LISTS ANDROID_CPP_SOURCES_ABS)
  file(RELATIVE_PATH rel_src "${CMAKE_CURRENT_SOURCE_DIR}" "${src}")
  list(APPEND ANDROID_CPP_SOURCES "${rel_src}")
endforeach()
foreach(src IN LISTS COMMON_CPP_SOURCES_ABS)
  file(RELATIVE_PATH rel_src "${CMAKE_CURRENT_SOURCE_DIR}" "${src}")
  list(APPEND COMMON_CPP_SOURCES "${rel_src}")
endforeach()
foreach(src IN LISTS DSP_CPP_SOURCES_ABS)
  file(RELATIVE_PATH rel_src "${CMAKE_CURRENT_SOURCE_DIR}" "${src}")
  list(APPEND DSP_CPP_SOURCES "${rel_src}")
endforeach()

if (RN_AUDIO_API_FFMPEG_DISABLED)
  list(REMOVE_ITEM COMMON_CPP_SOURCES
    "${COMMON_CPP_DIR}/audioapi/decoding/backends/FfmpegDecoder.cpp"
  )
endif()

set_source_files_properties(
  ${DSP_CPP_SOURCES}
  PROPERTIES
  COMPILE_FLAGS "-O3"
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set_source_files_properties(
    "${COMMON_CPP_DIR}/audioapi/dsp/r8brain/fft/pffft_double.c"
    PROPERTIES
    COMPILE_FLAGS "-Wno-#pragma-messages"
  )
endif()

set(INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include)
set(FFMPEG_INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include_ffmpeg)
set(EXTERNAL_DIR ${COMMON_CPP_DIR}/audioapi/external/android)
set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs)

add_library(react-native-audio-api SHARED ${ANDROID_CPP_SOURCES} ${COMMON_CPP_SOURCES})

# Debug-only allocation auditor used by BaseAudioContext::processGraph (mirrors iOS podspec).
target_sources(react-native-audio-api PRIVATE
  $<$<CONFIG:Debug>:${COMMON_CPP_DIR}/test/src/graph/AudioThreadGuard.cpp>
)

if (NOT RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED)
  foreach(lib IN ITEMS opus opusfile ogg vorbis vorbisenc vorbisfile crypto ssl)
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${EXTERNAL_DIR}/${ANDROID_ABI}/lib${lib}.a)
  endforeach()
endif()

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

# PUBLIC: extension packages consume `<audioapi/...>` via the published prefab.
target_include_directories(
  react-native-audio-api
  PUBLIC
    "${COMMON_CPP_DIR}"
    "${ANDROID_CPP_DIR}"
  PRIVATE
    "${INCLUDE_DIR}"
    "${FFMPEG_INCLUDE_DIR}"
    "${REACT_NATIVE_DIR}/ReactCommon"
    "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
    "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule"
    "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
)

if(NOT RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED)
  target_include_directories(
    react-native-audio-api
    PRIVATE
      "${INCLUDE_DIR}/opus"
      "${INCLUDE_DIR}/vorbis"
  )
endif()

set(LINK_LIBRARIES
  ReactAndroid::jsi
  fbjni::fbjni
  android
  log
  mediandk
  oboe::oboe
)

set(RN_VERSION_LINK_LIBRARIES
  ReactAndroid::reactnative
)

if (RN_AUDIO_API_FFMPEG_DISABLED)
  target_compile_definitions(
    react-native-audio-api
    PRIVATE RN_AUDIO_API_FFMPEG_DISABLED=1
  )
endif()

target_compile_definitions(
  react-native-audio-api
  PRIVATE $<$<CONFIG:Debug>:DEBUG>
)

# Forward the SIMD capability detected in the top-level android/CMakeLists.txt so
# the dsp/ vector math (VectorMath, WsolaTimeStretcher) compiles its NEON/SSE2
# intrinsic paths instead of the scalar fallback. arm64-v8a always has NEON and
# x86_64 always has SSE2, so these are safe baselines for their ABIs.
if(HAVE_ARM_NEON_INTRINSICS)
  target_compile_definitions(react-native-audio-api PRIVATE HAVE_ARM_NEON_INTRINSICS=1)
elseif(HAVE_X86_SSE2)
  target_compile_definitions(react-native-audio-api PRIVATE HAVE_X86_SSE2=1)
endif()

if (RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED)
  target_compile_definitions(
    react-native-audio-api
    PRIVATE
    RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED=1
    MA_NO_LIBOPUS=1
    MA_NO_LIBVORBIS=1
  )
endif()

target_link_libraries(react-native-audio-api
  ${LINK_LIBRARIES}
  ${RN_VERSION_LINK_LIBRARIES}
  z
)

if(NOT RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED)
  target_link_libraries(react-native-audio-api
    opusfile
    opus
    ogg
    vorbis
    vorbisenc
    vorbisfile
  )
endif()

if(NOT RN_AUDIO_API_FFMPEG_DISABLED)
  target_link_libraries(react-native-audio-api
    avcodec
    avformat
    avutil
    swresample
  )
endif()
