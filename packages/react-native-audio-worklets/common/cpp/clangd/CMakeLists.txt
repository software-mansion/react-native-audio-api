# Internal CMake script for generating compile_commands.json for clangd / VS Code C++ tooling.

cmake_minimum_required(VERSION 3.12.0)
project(rnaudioworklets-clangd LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(COMMON_CPP_DIR ${CMAKE_SOURCE_DIR}/..)
set(WORKLETS_PACKAGE_DIR ${CMAKE_SOURCE_DIR}/../../..)
set(REPO_ROOT_DIR ${CMAKE_SOURCE_DIR}/../../../../../)
set(REACT_NATIVE_DIR ${REPO_ROOT_DIR}/node_modules/react-native)
set(WORKLETS_DIR ${REPO_ROOT_DIR}/node_modules/react-native-worklets/Common/cpp)
set(AUDIO_API_COMMON_CPP_DIR ${REPO_ROOT_DIR}/packages/react-native-audio-api/common/cpp)

set(CODEGEN_JNI_DIR "${WORKLETS_PACKAGE_DIR}/android/build/generated/source/codegen/jni")
set(CODEGEN_IOS_DIR "${REPO_ROOT_DIR}/apps/fabric-example/ios/build/generated/ios/ReactCodegen")

file(GLOB_RECURSE AUDIO_WORKLETS_SOURCES CONFIGURE_DEPENDS
  "${COMMON_CPP_DIR}/audioworklets/*.cpp"
  "${COMMON_CPP_DIR}/audioworklets/*/*.cpp")

add_library(rnaudioworklets_cursor STATIC ${AUDIO_WORKLETS_SOURCES})

target_compile_definitions(rnaudioworklets_cursor PRIVATE RCT_NEW_ARCH_ENABLED)

set(FABRIC_EXAMPLE_IOS_DIR ${REPO_ROOT_DIR}/apps/fabric-example/ios)
set(PODS_HEADERS_DIR "${FABRIC_EXAMPLE_IOS_DIR}/Pods/Headers/Public")

target_include_directories(
  rnaudioworklets_cursor
  PRIVATE
  "${COMMON_CPP_DIR}"
  "${AUDIO_API_COMMON_CPP_DIR}"
  "${WORKLETS_DIR}"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/jsi"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${REACT_NATIVE_DIR}/ReactCommon/react/nativemodule/core"
  "${REACT_NATIVE_DIR}/ReactCommon/react/bridging"
  "${CODEGEN_JNI_DIR}/react/renderer/components/rnaudioworklets"
  "${CODEGEN_IOS_DIR}"
  "${PODS_HEADERS_DIR}/ReactNativeDependencies"
  "${PODS_HEADERS_DIR}/React-Core"
)
