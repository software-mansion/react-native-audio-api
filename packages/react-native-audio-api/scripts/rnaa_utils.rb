def check_if_worklets_enabled()
  validate_worklets_script = File.expand_path(File.join(__dir__, 'validate-worklets-version.js'))
  unless system("node \"#{validate_worklets_script}\"")
    # If the validation script fails, we assume worklets are not present or have an incompatible version
    return false
  end
  true
end

def try_to_parse_react_native_package_json(node_modules_dir)
  react_native_package_json_path = File.join(node_modules_dir, 'react-native/package.json')
  if !File.exist?(react_native_package_json_path)
    return nil
  end
  return JSON.parse(File.read(react_native_package_json_path))
end

def find_audio_api_config()
  result = {
    :worklets_enabled => nil,
    :react_native_common_dir => nil,
    :dynamic_frameworks_audio_api_dir => nil,
    :dynamic_frameworks_worklets_dir => nil
  }

  result[:worklets_enabled] = check_if_worklets_enabled()

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

  if result[:worklets_enabled] == true
    react_native_worklets_node_modules_dir = File.join(File.dirname(`cd "#{Pod::Config.instance.installation_root.to_s}" && node --print "require.resolve('react-native-worklets/package.json')"`), '..')
    react_native_worklets_dir_absolute = File.join(react_native_worklets_node_modules_dir, 'react-native-worklets')
    react_native_worklets_dir_relative = Pathname.new(react_native_worklets_dir_absolute).relative_path_from(pods_root).to_s
    result[:dynamic_frameworks_worklets_dir] = react_native_worklets_dir_relative
  end

  return result
end
