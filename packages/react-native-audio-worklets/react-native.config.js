// Autolinking for Android/iOS cxx module integration.
// Android: `android/build.gradle` runs Codegen; the app's autolinking CMake
// links `react_codegen_rnaudioworklets` + the static `react-native-audio-worklets`
// target from the root `CMakeLists.txt`. iOS: `RNAudioWorklets.podspec` +
// `NativeAudioWorkletsModuleProvider`.
module.exports = {
  dependency: {
    platforms: {
      ios: {},
      android: {
        cmakeListsPath: 'build/generated/source/codegen/jni/CMakeLists.txt',
        cxxModuleCMakeListsModuleName: 'react-native-audio-worklets',
        cxxModuleCMakeListsPath: '../CMakeLists.txt',
        cxxModuleHeaderName: 'NativeAudioWorkletsModule',
      },
    },
  },
};
