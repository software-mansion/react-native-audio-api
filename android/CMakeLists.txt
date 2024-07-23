cmake_minimum_required(VERSION 3.9.0)
project(react-native-audio-context)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 14)

if(ANDROID)
  add_definitions(-DANDROID)
endif()

include(../node_modules/react-native/ReactAndroid/cmake-utils/folly-flags.cmake)
add_compile_options(${folly_FLAGS})

include_directories(
        ../cpp
        ../cpp/AudioContext
        ../cpp/AudioDestinationNode
        ../cpp/OscillatorNode
        ../cpp/AudioNode
        src/main/cpp
        ../node_modules/react-native/ReactCommon/jsi
        ../node_modules/react-native/ReactAndroid/src/main/jni/react/jni
        ../node_modules/react-native/ReactAndroid/src/main/jni/third-party/folly
)

add_library(react-native-audio-context SHARED
        src/main/cpp/OnLoad.cpp
        src/main/cpp/AudioContext
        src/main/cpp/OscillatorNode
        src/main/cpp/AudioDestinationNode
        src/main/cpp/AudioNode

        ../cpp/AudioContext/AudioContextHostObject
        ../cpp/AudioContext/AudioContextWrapper.h
        ../cpp/AudioContext/android/AudioContextWrapper.cpp

        ../cpp/AudioDestinationNode/AudioDestinationNodeHostObject
        ../cpp/AudioDestinationNode/AudioDestinationNodeWrapper.h
        ../cpp/AudioDestinationNode/android/AudioDestinationNodeWrapper.cpp

        ../cpp/OscillatorNode/OscillatorNodeHostObject
        ../cpp/OscillatorNode/OscillatorNodeWrapper.h
        ../cpp/OscillatorNode/android/OscillatorNodeWrapper.cpp

        ../cpp/AudioNode/AudioNodeHostObject.h
        ../cpp/AudioNode/AudioNodeWrapper.h
        ../cpp/AudioNode/android/AudioNodeWrapper.cpp
)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

target_link_libraries(react-native-audio-context
        ReactAndroid::jsi
        ReactAndroid::reactnativejni
        fbjni::fbjni
        ReactAndroid::folly_runtime
        android
        log
)
