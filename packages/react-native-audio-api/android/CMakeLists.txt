cmake_minimum_required(VERSION 3.12.0)
project(react-native-audio-api)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Detect the processor and SIMD support
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(HAVE_ARM_NEON_INTRINSICS TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(HAVE_X86_SSE2 TRUE)
endif()

# default CMAKE_CXX_FLAGS: "-g -DANDROID -fdata-sections -ffunction-sections
# -funwind-tables -fstack-protector-strong -no-canonical-prefixes
# -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fstack-protector-all"
include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 80)
  include("${REACT_NATIVE_DIR}/ReactCommon/cmake-utils/react-native-flags.cmake")
  target_compile_reactnative_options(react-native-audio-api PRIVATE)
else()
  string(APPEND CMAKE_CXX_FLAGS " -fexceptions -frtti -std=c++${CMAKE_CXX_STANDARD} -Wall")
endif()

string(APPEND CMAKE_CXX_FLAGS " -fno-omit-frame-pointer")

if(${IS_NEW_ARCHITECTURE_ENABLED})
  string(APPEND CMAKE_CXX_FLAGS " -DRCT_NEW_ARCH_ENABLED")
endif()

set(ANDROID_CPP_DIR "${CMAKE_SOURCE_DIR}/src/main/cpp")
set(COMMON_CPP_DIR "${CMAKE_SOURCE_DIR}/../common/cpp")

add_subdirectory("${ANDROID_CPP_DIR}/audioapi")
