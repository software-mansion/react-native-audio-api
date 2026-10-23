def try_to_parse_react_native_package_json(node_modules_dir)
  react_native_package_json_path = File.join(node_modules_dir, 'react-native/package.json')
  if !File.exist?(react_native_package_json_path)
    return nil
  end
  return JSON.parse(File.read(react_native_package_json_path))
end

# Build options come from the app's package.json:
#   "react-native-audio-api": { "enableFFmpeg": true, "disableStaticExternalLibs": false }
# Bare apps write the section by hand; in Expo apps `expo prebuild` writes it from the plugin options.
# The Podfile directory is the app's ios/, so the app root is its parent.
def read_audio_api_build_options()
  package_json_path = File.join(Pod::Config.instance.installation_root.to_s, '..', 'package.json')
  options = File.exist?(package_json_path) ? (JSON.parse(File.read(package_json_path))['react-native-audio-api'] || {}) : {}
  return {
    :ffmpeg_enabled => boolean_build_option(options, 'enableFFmpeg'),
    :static_external_libs_disabled => boolean_build_option(options, 'disableStaticExternalLibs'),
  }
end

# `false` when the key is absent; anything but a JSON boolean fails the install rather than
# silently building the wrong configuration.
def boolean_build_option(options, key)
  return false unless options.key?(key)
  value = options[key]
  unless value == true || value == false
    raise "[AudioAPI] \"#{key}\" in package.json must be true or false, got #{value.inspect}."
  end
  return value
end

def find_audio_api_config()
  result = {
    :react_native_common_dir => nil,
    :dynamic_frameworks_audio_api_dir => nil
  }

  react_native_node_modules_dir = File.join(File.dirname(`cd "#{Pod::Config.instance.installation_root.to_s}" && node --print "require.resolve('react-native/package.json')"`), '..')
  react_native_json = try_to_parse_react_native_package_json(react_native_node_modules_dir)

  if react_native_json == nil
    # user configuration, just in case
    node_modules_dir = ENV["REACT_NATIVE_NODE_MODULES_DIR"]
    react_native_json = try_to_parse_react_native_package_json(node_modules_dir)
  end

  if react_native_json == nil
    raise '[AudioAPI] Unable to recognize your `react-native` version. Please set environmental variable with `react-native` location: `export REACT_NATIVE_NODE_MODULES_DIR="<path to react-native>" && pod install`.'
  end

  pods_root = Pod::Config.instance.project_pods_root
  react_native_common_dir_absolute = File.join(react_native_node_modules_dir, 'react-native', 'ReactCommon')
  react_native_common_dir_relative = Pathname.new(react_native_common_dir_absolute).relative_path_from(pods_root).to_s
  result[:react_native_common_dir] = react_native_common_dir_relative

  react_native_audio_api_dir_absolute = File.join(__dir__, '..')
  react_native_audio_api_dir_relative = Pathname.new(react_native_audio_api_dir_absolute).relative_path_from(pods_root).to_s
  result[:dynamic_frameworks_audio_api_dir] = react_native_audio_api_dir_relative

  return result
end
