require "json"
require_relative './scripts/rnaa_utils'

package_json = JSON.parse(File.read(File.join(__dir__, "package.json")))
$audio_api_config = find_audio_api_config()

$new_arch_enabled = ENV['RCT_NEW_ARCH_ENABLED'] == '1'
$RN_AUDIO_API_FFMPEG_DISABLED = ENV['DISABLE_AUDIOAPI_FFMPEG'].nil? ? false : ENV['DISABLE_AUDIOAPI_FFMPEG'] == '1' # false by default

fabric_flags = $new_arch_enabled ? '-DRCT_NEW_ARCH_ENABLED' : ''
version_flag = "-DAUDIOAPI_VERSION=#{package_json['version']}"
ios_min_version = '14.0'

worklets_enabled = $audio_api_config[:worklets_enabled]
worklets_preprocessor_flag = worklets_enabled ? '-DRN_AUDIO_API_ENABLE_WORKLETS=1' : ''

ffmpeg_flag = $RN_AUDIO_API_FFMPEG_DISABLED ? '-DRN_AUDIO_API_FFMPEG_DISABLED=1' : ''
skip_ffmpeg_argument = $RN_AUDIO_API_FFMPEG_DISABLED ? 'skipffmpeg' : ''

Pod::Spec.new do |s|
  s.name         = "RNAudioAPI"
  s.version      = package_json["version"]
  s.summary      = package_json["description"]
  s.homepage     = package_json["homepage"]
  s.license      = package_json["license"]
  s.authors      = package_json["author"]

  s.platforms    = { :ios => ios_min_version }
  s.source       = { :git => "https://github.com/software-mansion/react-native-audio-api.git", :tag => "#{s.version}" }

  s.subspec "audioapi" do |ss|
    ss.source_files = "common/cpp/audioapi/**/*.{cpp,c,h,hpp}"
    ss.exclude_files = $RN_AUDIO_API_FFMPEG_DISABLED ? ["common/cpp/audioapi/libs/ffmpeg/**"] : []
    ss.header_dir = "audioapi"
    ss.header_mappings_dir = "common/cpp/audioapi"

    ss.subspec "ios" do |sss|
      sss.source_files = "ios/audioapi/**/*.{mm,h,m,hpp}"
      sss.header_dir = "audioapi"
      sss.header_mappings_dir = "ios/audioapi"
    end

    ss.subspec "audioapi_dsp" do |sss|
      sss.source_files = "common/cpp/audioapi/dsp/**/*.{cpp}"
      sss.header_dir = "audioapi/dsp"
      sss.header_mappings_dir = "common/cpp/audioapi/dsp"
      sss.compiler_flags = "-O3"
    end
  end

  if worklets_enabled
    s.dependency 'RNWorklets'
  end

  s.ios.frameworks = 'Accelerate', 'AVFoundation', 'MediaPlayer'

  s.prepare_command = <<-CMD
    chmod +x scripts/download-prebuilt-binaries.sh
    scripts/download-prebuilt-binaries.sh ios #{skip_ffmpeg_argument}
  CMD

  # Assumes Pods dir is nested under ios project dir
  ios_dir = File.join(Pod::Config.instance.project_pods_root, '..')
  rn_audio_dir_relative = Pathname.new(__dir__).relative_path_from(ios_dir).to_s

  external_dir_relative = "common/cpp/audioapi/external"
  lib_dir = "$(PROJECT_DIR)/#{rn_audio_dir_relative}/#{external_dir_relative}/$(PLATFORM_NAME)"

  s.ios.vendored_frameworks = $RN_AUDIO_API_FFMPEG_DISABLED ? [] : [
    'common/cpp/audioapi/external/ffmpeg_ios/libavcodec.xcframework',
    'common/cpp/audioapi/external/ffmpeg_ios/libavformat.xcframework',
    'common/cpp/audioapi/external/ffmpeg_ios/libavutil.xcframework',
    'common/cpp/audioapi/external/ffmpeg_ios/libswresample.xcframework'
  ]

  s.pod_target_xcconfig = {
    "USE_HEADERMAP" => "YES",
    "DEFINES_MODULE" => "YES",
    "HEADER_SEARCH_PATHS" => [
      '"$(PODS_TARGET_SRCROOT)/ReactCommon"',
      '"$(PODS_TARGET_SRCROOT)"',
      '"$(PODS_ROOT)/RCT-Folly"',
      '"$(PODS_ROOT)/boost"',
      '"$(PODS_ROOT)/boost-for-react-native"',
      '"$(PODS_ROOT)/DoubleConversion"',
      '"$(PODS_ROOT)/Headers/Private/React-Core"',
      '"$(PODS_ROOT)/Headers/Private/Yoga"',
      "\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include\"",
      "\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include/opus\"",
      "\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include/vorbis\"",
    ]
    .concat($RN_AUDIO_API_FFMPEG_DISABLED ? [] : ["\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/ffmpeg_include\""])
    .concat(worklets_enabled ? ['"$(PODS_ROOT)/Headers/Public/RNWorklets"'] : [])
    .join(' '),
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
    "GCC_PREPROCESSOR_DEFINITIONS" => '$(inherited) HAVE_ACCELERATE=1',
    'OTHER_CFLAGS' => "$(inherited) #{fabric_flags} #{version_flag} #{worklets_preprocessor_flag} #{ffmpeg_flag}",
  }

  s.xcconfig = {
    "HEADER_SEARCH_PATHS" => [
      '"$(PODS_ROOT)/boost"',
      '"$(PODS_ROOT)/boost-for-react-native"',
      '"$(PODS_ROOT)/glog"',
      '"$(PODS_ROOT)/RCT-Folly"',
      '"$(PODS_ROOT)/Headers/Public/React-hermes"',
      '"$(PODS_ROOT)/Headers/Public/hermes-engine"',
      "\"$(PODS_ROOT)/#{$audio_api_config[:react_native_common_dir]}\"",
      "\"$(PODS_ROOT)/#{$audio_api_config[:dynamic_frameworks_audio_api_dir]}/ios\"",
      "\"$(PODS_ROOT)/#{$audio_api_config[:dynamic_frameworks_audio_api_dir]}/common/cpp\"",
    ]
    .concat(worklets_enabled ? [
      '"$(PODS_ROOT)/Headers/Public/RNWorklets"',
      "\"$(PODS_ROOT)/#{$audio_api_config[:dynamic_frameworks_worklets_dir]}/apple\"",
      "\"$(PODS_ROOT)/#{$audio_api_config[:dynamic_frameworks_worklets_dir]}/Common/cpp\""
    ] : [])
    .join(' '),
    'OTHER_LDFLAGS' => %W[
      $(inherited)
      -force_load #{lib_dir}/libopusfile.a
      -force_load #{lib_dir}/libopus.a
      -force_load #{lib_dir}/libogg.a
      -force_load #{lib_dir}/libvorbis.a
      -force_load #{lib_dir}/libvorbisenc.a
      -force_load #{lib_dir}/libvorbisfile.a
      -force_load #{lib_dir}/libssl.a
      -force_load #{lib_dir}/libcrypto.a
    ].join(" "),
  }
  # Use install_modules_dependencies helper to install the dependencies if React Native version >=0.71.0.
  # See https://github.com/facebook/react-native/blob/febf6b7f33fdb4904669f99d795eba4c0f95d7bf/scripts/cocoapods/new_architecture.rb#L79.
  install_modules_dependencies(s)
end
