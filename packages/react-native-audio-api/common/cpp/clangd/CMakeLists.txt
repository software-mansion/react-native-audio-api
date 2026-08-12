# internal cmake script for instructing how to build the audio api to generate compile_commands.json that cursor uses

cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(COMMON_CPP_DIR ${CMAKE_SOURCE_DIR}/..)

# Toggle: set to 1 to enable test mode, 0 for normal mode
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

file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp" "${COMMON_CPP_DIR}/audioapi/*.c" "${COMMON_CPP_DIR}/test/*.cpp")
list(REMOVE_ITEM COMMON_CPP_SOURCES
  "${COMMON_CPP_DIR}/audioapi/HostObjects/inputs/AudioRecorderHostObject.cpp"
)

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
  "${googletest_SOURCE_DIR}/googletest/include"
)

target_include_directories(rnaudioapi_cursor PUBLIC ${INCLUDE_LIBRARIES})

target_link_libraries(rnaudioapi_cursor ${LINK_LIBRARIES} gtest gmock gtest_main)

# Android JNI glue (android/src/main/cpp/audioapi) is intentionally NOT built
# here: those sources only compile correctly with the NDK's own clang++ given
# --target=...-linux-android and --sysroot=<ndk>/sysroot. Bolting the NDK
# sysroot onto the host AppleClang via a plain -I mixes NDK libc headers with
# macOS's libc++, which breaks even <string>/<cmath>. generate-and-copy.sh
# instead merges in the real compile commands Gradle already produced under
# android/.cxx/ (see that script for details).

# ============================================================================
# iOS Objective-C++ glue (ios/audioapi/ios) — OBJECT library purely so clangd
# gets a compile command per file; never actually linked.
# ============================================================================
if(APPLE)
  enable_language(OBJCXX)

  set(IOS_DIR ${COMMON_CPP_DIR}/../../ios)
  file(GLOB_RECURSE IOS_MM_SOURCES CONFIGURE_DEPENDS "${IOS_DIR}/audioapi/ios/*.mm")

  add_library(rnaudioapi_cursor_ios OBJECT ${IOS_MM_SOURCES})
  set_source_files_properties(${IOS_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-fobjc-arc")

  target_compile_definitions(rnaudioapi_cursor_ios PRIVATE
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
  endif()

  # React/Pod headers (React-Core, ReactCommon, ReactCodegen, ...) — rather
  # than re-deriving every pod's header path by hand, reuse whatever
  # `pod install` already resolved for the example app. Requires
  # `cd apps/fabric-example/ios && pod install` to have been run at least once.
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

  target_include_directories(
    rnaudioapi_cursor_ios
    PRIVATE
    "${COMMON_CPP_DIR}"
    "${IOS_DIR}"
    # CocoaPods' header_mappings_dir flattens ios/audioapi's own headers so
    # Xcode can resolve bare `#import <NativeAudioPlayer.h>`-style includes
    # via its headermap; a plain include path needs to point directly at the
    # containing folder to reproduce that.
    "${IOS_DIR}/audioapi/ios/core"
    "${IOS_DIR}/audioapi/ios/system"
    ${IOS_POD_INCLUDE_DIRS}
  )
endif()