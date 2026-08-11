require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

if ENV['RCT_NEW_ARCH_ENABLED'] != '1'
  raise "[RNAudioWorklets] requires React Native New Architecture. Set ENV['RCT_NEW_ARCH_ENABLED'] = '1' in your Podfile."
end

fabric_flags = '-DRCT_NEW_ARCH_ENABLED'
version_flag = "-DAUDIO_WORKLETS_VERSION=#{package['version']}"
ios_min_version = '14.0'

Pod::Spec.new do |s|
  s.name         = "RNAudioWorklets"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => ios_min_version }
  s.source       = { :git => "https://github.com/software-mansion/react-native-audio-api.git", :tag => "#{s.version}" }

  s.source_files      = "common/cpp/audioworklets/**/*.{cpp,h}", "ios/audioworklets/**/*.{mm,h}"
  s.header_dir        = "audioworklets"
  s.header_mappings_dir = "common/cpp/audioworklets"

  s.dependency 'RNAudioAPI'
  s.dependency 'RNWorklets'
  s.dependency 'React-jsi'

  s.pod_target_xcconfig = {
    "USE_HEADERMAP" => "YES",
    "DEFINES_MODULE" => "YES",
    "HEADER_SEARCH_PATHS" => [
      '"$(PODS_TARGET_SRCROOT)/common/cpp"',
      '"$(PODS_TARGET_SRCROOT)/ReactCommon"',
      '"$(PODS_TARGET_SRCROOT)"',
      '"$(PODS_ROOT)/RCT-Folly"',
      '"$(PODS_ROOT)/boost"',
      '"$(PODS_ROOT)/boost-for-react-native"',
      '"$(PODS_ROOT)/DoubleConversion"',
      '"$(PODS_ROOT)/Headers/Private/React-Core"',
      '"$(PODS_ROOT)/Headers/Private/Yoga"',
      '"$(PODS_ROOT)/Headers/Public/RNAudioAPI"',
      '"$(PODS_ROOT)/Headers/Public/RNWorklets"',
      '"$(PODS_ROOT)/Headers/Private/ReactCodegen"',
      '"$(PODS_ROOT)/../build/generated/ios/ReactCodegen"',
    ].join(' '),
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
    "OTHER_CFLAGS" => "$(inherited) #{fabric_flags} #{version_flag}",
  }

  install_modules_dependencies(s)
end
