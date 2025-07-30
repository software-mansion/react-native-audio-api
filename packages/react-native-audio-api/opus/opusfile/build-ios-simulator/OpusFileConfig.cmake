
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was OpusFileConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# Ported from CMakeFindDependencyMacro.cmake (finding configs and using pkgconfig as fallback)
set(cmake_quiet_arg)
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
  set(cmake_quiet_arg QUIET)
endif()
set(cmake_required_arg)
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
  set(cmake_required_arg REQUIRED)
endif()

find_package(Ogg CONFIG ${cmake_quiet_arg})
if(NOT TARGET Ogg::ogg)
  find_package(PkgConfig REQUIRED ${cmake_quiet_arg})
  pkg_check_modules(Ogg ${cmake_required_arg} ${cmake_quiet_arg} IMPORTED_TARGET ogg)
  set_target_properties(PkgConfig::Ogg PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(Ogg::ogg ALIAS PkgConfig::Ogg)
endif()

if (NOT TARGET Ogg::ogg)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE "${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency Ogg could not be found.")
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND False)
  return()
endif()

find_package(Opus CONFIG ${cmake_quiet_arg})
if(NOT TARGET Opus::opus)
  find_package(PkgConfig REQUIRED ${cmake_quiet_arg})
  pkg_check_modules(Opus ${cmake_required_arg} ${cmake_quiet_arg} IMPORTED_TARGET opus)
  set_target_properties(PkgConfig::Opus PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(Opus::opus ALIAS PkgConfig::Opus)
endif()

if (NOT TARGET Opus::opus)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE "${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency Opus could not be found.")
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND False)
  return()
endif()

set(cmake_fd_required_arg)
set(cmake_fd_quiet_arg)

if (NOT ON)
  include(CMakeFindDependencyMacro)
  find_dependency(OpenSSL)
endif()

# Including targets of opusfile
include("${CMAKE_CURRENT_LIST_DIR}/OpusFileTargets.cmake")

check_required_components(OpusFile)
