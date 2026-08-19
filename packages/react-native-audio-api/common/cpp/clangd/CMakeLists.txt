# internal cmake script for instructing how to build the audio api to generate compile_commands.json that cursor uses

cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(COMMON_CPP_DIR ${CMAKE_SOURCE_DIR}/..)

# =1 swaps real types for dummy stand-ins too (e.g. JsiBuffer), so production
# code needs =0; only test/*.cpp needs =1 — split via separate targets below.
set(RN_AUDIO_API_TEST 0)
set(REACT_NATIVE_DIR ../../../../../node_modules/react-native)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows/cross-platform compatibility, force shared runtime matching
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# -------------------------------------

file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c")
list(REMOVE_ITEM COMMON_CPP_SOURCES
  "${COMMON_CPP_DIR}/audioapi/HostObjects/inputs/AudioRecorderHostObject.cpp"
)
file(GLOB_RECURSE TEST_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/test/*.cpp")

set(INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include)
set(FFMPEG_INCLUDE_DIR ${COMMON_CPP_DIR}/audioapi/external/include_ffmpeg)
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

set(COMMON_EXTRA_INCLUDES
  "${COMMON_CPP_DIR}"
  "${INCLUDE_DIR}"
  "${INCLUDE_DIR}/opus"
  "${FFMPEG_INCLUDE_DIR}"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/jsi"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${COMMON_CPP_DIR}/../../ios"
  "${googletest_SOURCE_DIR}/googletest/include"
)
target_include_directories(rnaudioapi_cursor PRIVATE ${COMMON_EXTRA_INCLUDES})

target_include_directories(rnaudioapi_cursor PUBLIC ${INCLUDE_LIBRARIES})

target_link_libraries(rnaudioapi_cursor ${LINK_LIBRARIES} gtest gmock gtest_main)

# Mirrors common/cpp/test/CMakeLists.txt's definitions exactly.
add_library(rnaudioapi_cursor_test OBJECT ${TEST_CPP_SOURCES})
target_compile_definitions(rnaudioapi_cursor_test PRIVATE
  RN_AUDIO_API_TEST=1
  RN_AUDIO_API_FFMPEG_DISABLED=1
  RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED=1
  MA_NO_LIBOPUS=1
  MA_NO_LIBVORBIS=1
)
target_include_directories(rnaudioapi_cursor_test PRIVATE ${COMMON_EXTRA_INCLUDES})
# OBJECT libs never link, but this still pulls in gtest/gmock's include dirs.
target_link_libraries(rnaudioapi_cursor_test PRIVATE gtest gmock gtest_main)

# Headers have no compile command of their own, so clangd falls back to
# "inferred from <whatever .cpp/.mm you most recently opened>" — session-order
# dependent and breaks badly across platforms (e.g. borrowed iOS flags can
# lose <span>). Force real per-header entries instead of relying on that.
file(GLOB_RECURSE COMMON_CPP_HEADERS CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.h" "${COMMON_CPP_DIR}/audioapi/*.hpp")
# Vendored code isn't self-contained (e.g. FFmpeg needs -D__STDC_CONSTANT_MACROS
# from its caller) and was never meant to compile standalone.
list(FILTER COMMON_CPP_HEADERS EXCLUDE REGEX "/audioapi/external/")
list(FILTER COMMON_CPP_HEADERS EXCLUDE REGEX "/audioapi/libs/")
list(FILTER COMMON_CPP_HEADERS EXCLUDE REGEX "/audioapi/dsp/r8brain/")

add_library(rnaudioapi_cursor_headers OBJECT ${COMMON_CPP_HEADERS})
set_source_files_properties(${COMMON_CPP_HEADERS} PROPERTIES
  HEADER_FILE_ONLY OFF
  LANGUAGE CXX
)
target_compile_definitions(rnaudioapi_cursor_headers PRIVATE
  RN_AUDIO_API_TEST=${RN_AUDIO_API_TEST}
)
target_include_directories(rnaudioapi_cursor_headers PRIVATE ${COMMON_EXTRA_INCLUDES})

# Android JNI glue (android/src/main/cpp/audioapi) isn't built here: it only
# compiles correctly with the NDK's own clang++ (--target/--sysroot); bolting
# the NDK sysroot onto AppleClang breaks even <string>/<cmath>.
# generate-and-copy.sh merges in Gradle's real android/.cxx/ commands instead.

# iOS Objective-C++ glue (ios/audioapi/ios) — OBJECT libs purely for compile
# commands; never actually linked.
if(APPLE)
  enable_language(OBJCXX)
  enable_language(OBJC)

  set(IOS_DIR ${COMMON_CPP_DIR}/../../ios)
  # Podspec compiles both .mm and plain .m (NativeAudioPlayer.m etc). Separate
  # targets: a shared lib would leak CMAKE_CXX_STANDARD's -std=c++20 onto .m,
  # which clang rejects outright for plain Objective-C.
  file(GLOB_RECURSE IOS_MM_SOURCES CONFIGURE_DEPENDS "${IOS_DIR}/audioapi/ios/*.mm")
  file(GLOB_RECURSE IOS_M_SOURCES CONFIGURE_DEPENDS "${IOS_DIR}/audioapi/ios/*.m")

  add_library(rnaudioapi_cursor_ios OBJECT ${IOS_MM_SOURCES})
  add_library(rnaudioapi_cursor_ios_objc OBJECT ${IOS_M_SOURCES})
  set_source_files_properties(${IOS_MM_SOURCES} ${IOS_M_SOURCES} PROPERTIES COMPILE_FLAGS "-fobjc-arc")

  target_compile_definitions(rnaudioapi_cursor_ios PRIVATE
    RN_AUDIO_API_TEST=${RN_AUDIO_API_TEST}
  )
  target_compile_definitions(rnaudioapi_cursor_ios_objc PRIVATE
    RN_AUDIO_API_TEST=${RN_AUDIO_API_TEST}
  )

  # System frameworks (Foundation, AVFoundation, AudioToolbox, ...) are found
  # automatically once -isysroot points at an SDK.
  execute_process(
    COMMAND xcrun --sdk iphonesimulator --show-sdk-path
    OUTPUT_VARIABLE IOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(IOS_SDK_PATH)
    target_compile_options(rnaudioapi_cursor_ios PRIVATE -isysroot "${IOS_SDK_PATH}")
    target_compile_options(rnaudioapi_cursor_ios_objc PRIVATE -isysroot "${IOS_SDK_PATH}")
  endif()

  # Reuse whatever `pod install` already resolved for the example app rather
  # than re-deriving every pod's header path by hand. Requires having run it.
  set(FABRIC_EXAMPLE_IOS_DIR ${COMMON_CPP_DIR}/../../../../apps/fabric-example/ios)
  set(PODS_ROOT "${FABRIC_EXAMPLE_IOS_DIR}/Pods")
  set(PODS_XCCONFIG "${FABRIC_EXAMPLE_IOS_DIR}/Pods/Target Support Files/Pods-FabricExample/Pods-FabricExample.debug.xcconfig")

  set(IOS_POD_INCLUDE_DIRS "")
  if(EXISTS "${PODS_XCCONFIG}")
    file(STRINGS "${PODS_XCCONFIG}" HEADER_SEARCH_LINE REGEX "^HEADER_SEARCH_PATHS")
    string(REGEX REPLACE "^HEADER_SEARCH_PATHS *= *" "" HEADER_SEARCH_LINE "${HEADER_SEARCH_LINE}")
    string(REPLACE "\"" "" HEADER_SEARCH_LINE "${HEADER_SEARCH_LINE}")
    string(REPLACE "$(inherited)" "" HEADER_SEARCH_LINE "${HEADER_SEARCH_LINE}")
    string(REPLACE "\${PODS_ROOT}" "${PODS_ROOT}" HEADER_SEARCH_LINE "${HEADER_SEARCH_LINE}")
    string(REPLACE "$(PODS_ROOT)" "${PODS_ROOT}" HEADER_SEARCH_LINE "${HEADER_SEARCH_LINE}")
    separate_arguments(IOS_POD_INCLUDE_DIRS UNIX_COMMAND "${HEADER_SEARCH_LINE}")
  else()
    message(WARNING "clangd/CMakeLists.txt: ${PODS_XCCONFIG} not found — run `pod install` in apps/fabric-example/ios to resolve React/Pod headers for iOS files.")
  endif()

  set(IOS_EXTRA_INCLUDES
    "${COMMON_CPP_DIR}"
    "${IOS_DIR}"
    # Xcode's headermap resolves bare `#import <NativeAudioPlayer.h>` via
    # CocoaPods' header_mappings_dir; a plain -I needs the containing folder.
    "${IOS_DIR}/audioapi/ios/core"
    "${IOS_DIR}/audioapi/ios/system"
    ${IOS_POD_INCLUDE_DIRS}
  )
  target_include_directories(rnaudioapi_cursor_ios PRIVATE ${IOS_EXTRA_INCLUDES})
  target_include_directories(rnaudioapi_cursor_ios_objc PRIVATE ${IOS_EXTRA_INCLUDES})

  # Same reasoning as the common headers above. These headers also rely on
  # their .mm caller having already #imported AVFoundation (see the
  # `#ifndef __OBJC__` guard in e.g. IOSRecorderCallback.h) — `-include`
  # reproduces that here.
  file(GLOB_RECURSE IOS_HEADERS CONFIGURE_DEPENDS "${IOS_DIR}/audioapi/ios/*.h")

  add_library(rnaudioapi_cursor_ios_headers OBJECT ${IOS_HEADERS})
  set_source_files_properties(${IOS_HEADERS} PROPERTIES
    HEADER_FILE_ONLY OFF
    LANGUAGE OBJCXX
    COMPILE_FLAGS "-include AVFoundation/AVFoundation.h"
  )
  target_compile_definitions(rnaudioapi_cursor_ios_headers PRIVATE
    RN_AUDIO_API_TEST=${RN_AUDIO_API_TEST}
  )
  target_include_directories(rnaudioapi_cursor_ios_headers PRIVATE ${IOS_EXTRA_INCLUDES})
  if(IOS_SDK_PATH)
    target_compile_options(rnaudioapi_cursor_ios_headers PRIVATE -isysroot "${IOS_SDK_PATH}")
  endif()
endif()