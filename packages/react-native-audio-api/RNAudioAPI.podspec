require "json"
require_relative './scripts/rnaa_utils'

package_json = JSON.parse(File.read(File.join(__dir__, "package.json")))
$audio_api_config = find_audio_api_config()

# Like React Native since 0.76, only an explicit RCT_NEW_ARCH_ENABLED=0 selects the legacy architecture.
$new_arch_enabled = ENV['RCT_NEW_ARCH_ENABLED'] != '0'
$audio_api_build_options = read_audio_api_build_options()
$RN_AUDIO_API_FFMPEG_ENABLED = $audio_api_build_options[:ffmpeg_enabled]
$RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED = $audio_api_build_options[:static_external_libs_disabled]

fabric_flags = $new_arch_enabled ? '-DRCT_NEW_ARCH_ENABLED' : ''
version_flag = "-DAUDIOAPI_VERSION=#{package_json['version']}"
ios_min_version = '14.0'

ffmpeg_libs = %w[libavcodec libavformat libavutil libswresample]
ffmpeg_flag = $RN_AUDIO_API_FFMPEG_ENABLED ? '-DRN_AUDIO_API_FFMPEG_ENABLED=1' : ''
ffmpeg_ldflags = $RN_AUDIO_API_FFMPEG_ENABLED ? ffmpeg_libs.map { |lib| "-framework \"#{lib}\"" }.join(' ') : ''
static_external_libs_flag = $RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED ? '-DRN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED=1 -DMA_NO_LIBOPUS=1 -DMA_NO_LIBVORBIS=1' : ''
skip_ffmpeg_argument = $RN_AUDIO_API_FFMPEG_ENABLED ? '' : 'skipffmpeg'
download_script_env_prefix = $RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED ? 'DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS=1 ' : ''

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
    ss.exclude_files = $RN_AUDIO_API_FFMPEG_ENABLED ? [] : ["common/cpp/audioapi/decoding/backends/FfmpegDecoder.cpp"]
    ss.header_dir = "audioapi"
    ss.header_mappings_dir = "common/cpp/audioapi"

    ss.subspec "ios" do |sss|
      sss.source_files = "ios/audioapi/**/*.{mm,h,m,hpp}"
      sss.header_dir = "audioapi"
      sss.header_mappings_dir = "ios/audioapi"
    end

    ss.subspec "audioapi_dsp" do |sss|
      sss.source_files = "common/cpp/audioapi/dsp/**/*.{cpp,inc}"
      sss.header_dir = "audioapi/dsp"
      sss.header_mappings_dir = "common/cpp/audioapi/dsp"
      sss.compiler_flags = "-O3"
    end

    # compile miniaudio implementation file as objective-c++
    ss.subspec "miniaudio_impl" do |sss|
      sss.source_files = "common/cpp/audioapi/utils/MiniaudioImplementation.cpp"
      sss.header_dir = "audioapi/libs"
      sss.header_mappings_dir = "common/cpp/audioapi/libs"
      sss.compiler_flags = "-x objective-c++"
    end
  end

  s.ios.frameworks = 'Accelerate', 'AVFoundation', 'AudioToolbox', 'CoreGraphics', 'ImageIO', 'MediaPlayer', 'UIKit'

  s.prepare_command = <<-CMD
    chmod +x scripts/download-prebuilt-binaries.sh
    #{download_script_env_prefix}scripts/download-prebuilt-binaries.sh ios #{skip_ffmpeg_argument}
  CMD

  external_dir_relative = "common/cpp/audioapi/external"
  xcframeworks_dir = "$(PODS_ROOT)/#{$audio_api_config[:dynamic_frameworks_audio_api_dir]}/#{external_dir_relative}/xcframeworks"

  # Every prebuilt library is a `<lib>.xcframework` in `external/xcframeworks`, the same assets
  # Package.swift uses; CocoaPods picks the device / simulator / Catalyst slice.
  ffmpeg_xcframeworks = ffmpeg_libs.map { |lib| "#{lib}.xcframework" }
  static_external_lib_xcframeworks = %w[libopusfile libopus libogg libvorbis libvorbisenc libvorbisfile]
    .map { |lib| "#{lib}.xcframework" }
  vendored_xcframeworks = []
  vendored_xcframeworks.concat(static_external_lib_xcframeworks) unless $RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
  vendored_xcframeworks.concat(ffmpeg_xcframeworks) if $RN_AUDIO_API_FFMPEG_ENABLED

  # `prepare_command` hydrates binaries on a fresh `pod install` so CocoaPods can
  # register all vendored `.xcframework`s in `[CP] Copy XCFrameworks`. When `Pods/`
  # is restored from cache while `node_modules/` is reinstalled, `prepare_command`
  # does not re-run — the build-time phase below re-hydrates on every build.
  #
  # It must run at `:before_headers` so it lands ahead of CocoaPods' own
  # `[CP] Copy XCFrameworks` phase, which consumes the xcframeworks. If it
  # ran later (e.g. `:before_compile`, which is after Copy XCFrameworks), Xcode
  # would reject the build when the xcframeworks are missing before we download.
  #
  # `output_files` declares everything Copy XCFrameworks consumes, so Xcode's
  # build graph knows this phase produces them.
  unless vendored_xcframeworks.empty?
    download_output_files = vendored_xcframeworks.map { |xcframework| "#{xcframeworks_dir}/#{xcframework}" }

    s.script_phase = {
      :name => 'Download RNAudioAPI prebuilt binaries',
      :execution_position => :before_headers,
      :always_out_of_date => '1',
      :output_files => download_output_files,
      :script => <<-SCRIPT
        cd "$PODS_TARGET_SRCROOT"
        chmod +x scripts/download-prebuilt-binaries.sh
        #{download_script_env_prefix}scripts/download-prebuilt-binaries.sh ios #{skip_ffmpeg_argument}
      SCRIPT
    }
  end

  s.ios.vendored_frameworks = vendored_xcframeworks.map { |xcframework| "#{external_dir_relative}/xcframeworks/#{xcframework}" }

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
    ]
    .concat($RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED ? [] : [
      "\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include/opus\"",
      "\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include/vorbis\""
    ])
    .concat($RN_AUDIO_API_FFMPEG_ENABLED ? ["\"$(PODS_TARGET_SRCROOT)/#{external_dir_relative}/include_ffmpeg\""] : [])
    .join(' '),
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
    "GCC_PREPROCESSOR_DEFINITIONS" => '$(inherited) HAVE_ACCELERATE=1',
    "GCC_PREPROCESSOR_DEFINITIONS[config=Debug]" => '$(inherited) HAVE_ACCELERATE=1 DEBUG=1',
    "EXCLUDED_SOURCE_FILE_NAMES[config=Release]" => 'AudioThreadGuard.cpp',
    'OTHER_CFLAGS' => "$(inherited) #{fabric_flags} #{version_flag} #{ffmpeg_flag} #{static_external_libs_flag}",
    # CocoaPods links a pod's own vendored dynamic frameworks only into the app target. A dynamic
    # RNAudioAPI framework (`use_frameworks! :linkage => :dynamic`) must link FFmpeg itself; the
    # frameworks ship no module maps, so clang cannot auto-link them.
    'OTHER_LDFLAGS' => "$(inherited) #{ffmpeg_ldflags}",
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
    .join(' '),
  }
  # Use install_modules_dependencies helper to install the dependencies if React Native version >=0.71.0.
  # See https://github.com/facebook/react-native/blob/febf6b7f33fdb4904669f99d795eba4c0f95d7bf/scripts/cocoapods/new_architecture.rb#L79.
  install_modules_dependencies(s)
end
