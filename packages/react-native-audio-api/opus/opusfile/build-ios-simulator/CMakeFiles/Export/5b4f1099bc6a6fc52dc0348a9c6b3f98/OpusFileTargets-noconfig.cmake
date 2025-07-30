#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpusFile::opusfile" for configuration ""
set_property(TARGET OpusFile::opusfile APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OpusFile::opusfile PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libopusfile.a"
  )

list(APPEND _cmake_import_check_targets OpusFile::opusfile )
list(APPEND _cmake_import_check_files_for_OpusFile::opusfile "${_IMPORT_PREFIX}/lib/libopusfile.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
