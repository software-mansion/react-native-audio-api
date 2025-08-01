require "json"

package_json = JSON.parse(File.read(File.join(__dir__, "package.json")))

$new_arch_enabled = ENV['RCT_NEW_ARCH_ENABLED'] == '1'

folly_flags = "-DFOLLY_NO_CONFIG -DFOLLY_MOBILE=1 -DFOLLY_USE_LIBCPP=1 -Wno-comma -Wno-shorten-64-to-32"
fabric_flags = $new_arch_enabled ? '-DRCT_NEW_ARCH_ENABLED' : ''
version_flag = "-DAUDIOAPI_VERSION=#{package_json['version']}"

Pod::Spec.new do |s|
  s.name         = "RNAudioAPI"
  s.version      = package_json["version"]
  s.summary      = package_json["description"]
  s.homepage     = package_json["homepage"]
  s.license      = package_json["license"]
  s.authors      = package_json["author"]

  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/software-mansion/react-native-audio-api.git", :tag => "#{s.version}" }

  s.subspec "audioapi" do |ss|
    ss.source_files = "common/cpp/audioapi/**/*.{cpp,c,h}"
    ss.header_dir = "audioapi"
    ss.header_mappings_dir = "common/cpp/audioapi"

    ss.subspec "ios" do |sss|
      sss.source_files = "ios/audioapi/**/*.{mm,h,m}"
      sss.header_dir = "audioapi"
      sss.header_mappings_dir = "ios/audioapi"
    end
  end

  s.ios.frameworks = 'CoreFoundation', 'CoreAudio', 'AudioToolbox', 'Accelerate', 'MediaPlayer', 'AVFoundation'

  s.compiler_flags = "#{folly_flags}"

  s.pod_target_xcconfig = {
    "USE_HEADERMAP" => "YES",
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
    "GCC_PREPROCESSOR_DEFINITIONS" => '$(inherited) HAVE_ACCELERATE=1',
    "HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/common/cpp\" \"$(PODS_TARGET_SRCROOT)/ios\" \"$(PODS_TARGET_SRCROOT)/opus/ogg/include\" \"$(PODS_TARGET_SRCROOT)/opus/opus/include\"",
    'OTHER_CFLAGS' => "$(inherited) #{folly_flags} #{fabric_flags} #{version_flag}"
  }

  s.user_target_xcconfig = {
    'OTHER_LDFLAGS' => "$(inherited) -force_load \"$(SRCROOT)/../../../packages/react-native-audio-api/opus/opusfile/build-ios-simulator/libopusfile.a\ -force_load \"$(SRCROOT)/../../../packages/react-native-audio-api/opus/opus/build-ios-simulator/libopus.a\ -force_load \"$(SRCROOT)/../../../packages/react-native-audio-api/opus/ogg/build-ios-simulator/libogg.a\""
  }

  # Use install_modules_dependencies helper to install the dependencies if React Native version >=0.71.0.
  # See https://github.com/facebook/react-native/blob/febf6b7f33fdb4904669f99d795eba4c0f95d7bf/scripts/cocoapods/new_architecture.rb#L79.
  install_modules_dependencies(s)
end


