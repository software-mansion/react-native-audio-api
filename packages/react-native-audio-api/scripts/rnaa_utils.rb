def check_if_worklets_enabled()
  validate_worklets_script = File.expand_path(File.join(__dir__, 'validate-worklets-version.js'))
  unless system("node \"#{validate_worklets_script}\"")
    # If the validation script fails, we assume worklets are not present or have an incompatible version
    return false
  end
  true
end
