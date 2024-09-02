cmake_minimum_required(VERSION 3.9.0)
project(react-native-audio-context)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 14)

if(ANDROID)
  add_definitions(-DANDROID)
endif()

include(../node_modules/react-native/ReactAndroid/cmake-utils/folly-flags.cmake)
add_compile_options(${folly_FLAGS})

set(INCLUDE_DIRS
    ../cpp
    ../cpp/AudioContext
    ../cpp/AudioDestinationNode
    ../cpp/OscillatorNode
    ../cpp/AudioNode
    ../cpp/GainNode
    ../cpp/StereoPannerNode
    ../cpp/AudioParam
    ../cpp/BiquadFilterNode
    ../cpp/AudioBuffer
    ../cpp/AudioBufferSourceNode

    src/main/cpp
    src/main/cpp/AudioContext
    src/main/cpp/AudioDestinationNode
    src/main/cpp/OscillatorNode
    src/main/cpp/AudioNode
    src/main/cpp/GainNode
    src/main/cpp/StereoPannerNode
    src/main/cpp/AudioParam
    src/main/cpp/BiquadFilterNode
    src/main/cpp/AudioBuffer
    src/main/cpp/AudioBufferSourceNode

    ../node_modules/react-native/ReactCommon/jsi
    ../node_modules/react-native/ReactAndroid/src/main/jni/react/jni
    ../node_modules/react-native/ReactAndroid/src/main/jni/third-party/folly
)

# Include directories
include_directories(${INCLUDE_DIRS})

file(GLOB_RECURSE SOURCE_FILES
        "src/main/cpp/*.cpp"
        "src/main/cpp/*.h"
        "../cpp/*.cpp"
        "../cpp/*.h"
)

add_library(react-native-audio-context SHARED ${SOURCE_FILES})

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

set(LINK_LIBRARIES
        ReactAndroid::jsi
        ReactAndroid::reactnativejni
        fbjni::fbjni
        ReactAndroid::folly_runtime
        android
        log
)

target_link_libraries(react-native-audio-context ${LINK_LIBRARIES})
