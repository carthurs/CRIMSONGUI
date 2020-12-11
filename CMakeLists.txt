# Top level CMake build script for CRIMSON.
# 
# Note that this script is used for both:
# - Top level Superbuild configuration
# - Crimson-Configure project for building the CRIMSON application itself.

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Change project and application name to your own
set(MY_PROJECT_NAME CRIMSON)
set(MY_APP_NAME CRIMSON)

set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)

#-----------------------------------------------------------------------------
# Set a default build type if none was specified
#-----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Release recommended)." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
               STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
# Superbuild Cache variable
# 
# This is more than just a user-modifiable setting, if you set this to ON,
# the build scripts will:
# 1. run this script once with it ON (and download the external projects)
# 2. run this script once with it OFF (to compile the CRIMSON application)
#-----------------------------------------------------------------------------

option(${MY_PROJECT_NAME}_USE_SUPERBUILD "Build ${MY_PROJECT_NAME} and the projects it depends on via SuperBuild.cmake (it's strongly recommended that you leave this ON for CRIMSON)." ON)

if(${MY_PROJECT_NAME}_USE_SUPERBUILD)
  project(${MY_PROJECT_NAME}-superbuild)
  set(${MY_PROJECT_NAME}_SOURCE_DIR ${PROJECT_SOURCE_DIR})
  set(${MY_PROJECT_NAME}_BINARY_DIR ${PROJECT_BINARY_DIR})
else()
  project(${MY_PROJECT_NAME})
endif()

#-----------------------------------------------------------------------------
# See http://cmake.org/cmake/help/cmake-2-8-docs.html#section_Policies for details
#-----------------------------------------------------------------------------

set(project_policies
  CMP0001 # NEW: CMAKE_BACKWARDS_COMPATIBILITY should no longer be used.
  CMP0002 # NEW: Logical target names must be globally unique.
  CMP0003 # NEW: Libraries linked via full path no longer produce linker search paths.
  CMP0004 # NEW: Libraries linked may NOT have leading or trailing whitespace.
  CMP0005 # NEW: Preprocessor definition values are now escaped automatically.
  CMP0006 # NEW: Installing MACOSX_BUNDLE targets requires a BUNDLE DESTINATION.
  CMP0007 # NEW: List command no longer ignores empty elements.
  CMP0008 # NEW: Libraries linked by full-path must have a valid library file name.
  CMP0009 # NEW: FILE GLOB_RECURSE calls should not follow symlinks by default.
  CMP0010 # NEW: Bad variable reference syntax is an error.
  CMP0011 # NEW: Included scripts do automatic cmake_policy PUSH and POP.
  CMP0012 # NEW: if() recognizes numbers and boolean constants.
  CMP0013 # NEW: Duplicate binary directories are not allowed.
  CMP0014 # NEW: Input directories must have CMakeLists.txt
  CMP0020 # NEW: Automatically link Qt executables to qtmain target on Windows
  CMP0028 # NEW: Double colon in target name means ALIAS or IMPORTED target.
  CMP0043 # NEW: Ignore COMPILE_DEFINITIONS_<Config> properties.
  )
foreach(policy ${project_policies})
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Update CMake module path
#------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${${MY_PROJECT_NAME}_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH} )
message("Added {{MY_PROJECT_NAME}_SOURCE_DIR}/CMake=" ${${MY_PROJECT_NAME}_SOURCE_DIR}/CMake " CMAKE_MODULE_PATH is now " ${CMAKE_MODULE_PATH})



#-----------------------------------------------------------------------------
# Output directories.
#-----------------------------------------------------------------------------

foreach(type LIBRARY RUNTIME ARCHIVE)
  set(output_dir ${${MY_PROJECT_NAME}_BINARY_DIR}/bin)
  set(CMAKE_${type}_OUTPUT_DIRECTORY ${output_dir} CACHE INTERNAL "Single output directory for building all libraries.")
  mark_as_advanced(CMAKE_${type}_OUTPUT_DIRECTORY)
endforeach()

#-----------------------------------------------------------------------------
# Additional Settings (also shown during superbuild)
#-----------------------------------------------------------------------------

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE PATH "")

#-----------------------------------------------------------------------------
# Additional Options (also shown during superbuild)
#-----------------------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build ${MY_PROJECT_NAME} with shared libraries" ON)
option(WITH_COVERAGE "Enable/Disable coverage" OFF)
option(BUILD_TESTING "Test the project" ON)

option(${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS "Build all ${MY_PROJECT_NAME} plugins" OFF)
option(${MY_PROJECT_NAME}_BUILD_ALL_APPS "Build all ${MY_PROJECT_NAME} applications" OFF)

mark_as_advanced(${MY_PROJECT_NAME}_INSTALL_RPATH_RELATIVE
                 ${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS
                 ${MY_PROJECT_NAME}_BUILD_ALL_APPS
                 )
                 
option(${MY_PROJECT_NAME}_BUILD_TRIAL_VERSION "Build the trial version" OFF)                 

# Meshing kernel selection
set(CRIMSON_MESHING_KERNEL "CGALVMTK" CACHE STRING "The type of meshing kernel")
set_property(CACHE CRIMSON_MESHING_KERNEL PROPERTY STRINGS "CGALVMTK;MeshSim")

message("{MY_PROJECT_NAME}_USE_SUPERBUILD is " ${${MY_PROJECT_NAME}_USE_SUPERBUILD})
message("MY_PROJECT_NAME is " ${MY_PROJECT_NAME})
if(${MY_PROJECT_NAME}_USE_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
else()
  include("${CMAKE_CURRENT_SOURCE_DIR}/NonSuperBuild.cmake")
endif()

# TODO: Eventually I think a better CPack implementation would use something like this,
#       After both the superbuild and non-superbuild have been run through, then include this file
#       and have this script package it, NOT Crimson-Build because Crimson-Build does not know where
#       all of the dependencies are.
# include("{CMAKE_CURRENT_SOURCE_DIR}/CPack_Package_All.cmake")

message("End of ./CMakeLists.txt")