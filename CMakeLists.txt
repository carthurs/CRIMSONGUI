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
  message(STATUS "Setting build type to 'Debug' as none was specified.")
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

set(CMAKE_MODULE_PATH
  ${${MY_PROJECT_NAME}_SOURCE_DIR}/CMake
  ${CMAKE_MODULE_PATH}
  )

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

include(MacroEmptyExternalProject)

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
set_property(CACHE CRIMSON_MESHING_KERNEL PROPERTY STRINGS "CGALVMTK")

#-----------------------------------------------------------------------------
# Superbuild script
#-----------------------------------------------------------------------------

if(${MY_PROJECT_NAME}_USE_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#*****************************************************************************
#**************************** END OF SUPERBUILD ****************************
#*****************************************************************************

#-----------------------------------------------------------------------------
# Prerequesites
#-----------------------------------------------------------------------------

find_package(MITK 2016.11.0 REQUIRED)

if(COMMAND mitkFunctionCheckMitkCompatibility)
  mitkFunctionCheckMitkCompatibility(VERSIONS MITK_VERSION_PLUGIN_SYSTEM 1 REQUIRED)
else()
  message(SEND_ERROR "Your MITK version is too old. Please use Git hash b86bf28 or newer")
endif()

link_directories(${MITK_LINK_DIRECTORIES})

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

set(CMAKE_MODULE_PATH
  ${MITK_SOURCE_DIR}/CMake
  ${CMAKE_MODULE_PATH}
  )

include(mitkFunctionCheckCompilerFlags)
include(mitkFunctionGetGccVersion)
include(mitkFunctionGetVersion)
include(mitkFunctionConfigureVisualStudioUserProjectFile)
include(crimsonFunctionCreateBlueBerryApplication)
include(crimsonFunctionConfigureVisualStudioUserProjectFile)

#-----------------------------------------------------------------------------
# Set project specific options and variables (NOT available during superbuild)
#-----------------------------------------------------------------------------

set(${MY_PROJECT_NAME}_VERSION_MAJOR "2020")
set(${MY_PROJECT_NAME}_VERSION_MINOR "11")
set(${MY_PROJECT_NAME}_VERSION_PATCH "30")   
set(${MY_PROJECT_NAME}_VERSION_STRING "${${MY_PROJECT_NAME}_VERSION_MAJOR}.${${MY_PROJECT_NAME}_VERSION_MINOR}.${${MY_PROJECT_NAME}_VERSION_PATCH}")

# Ask the user if a console window should be shown with the applications
option(${MY_PROJECT_NAME}_SHOW_CONSOLE_WINDOW "Use this to enable or disable the console window when starting GUI Applications" ON)
mark_as_advanced(${MY_PROJECT_NAME}_SHOW_CONSOLE_WINDOW)

if(NOT UNIX AND NOT MINGW)
  set(MITK_WIN32_FORCE_STATIC "STATIC")
endif()

set(${MY_PROJECT_NAME}_MODULES_PACKAGE_DEPENDS_DIR "${PROJECT_SOURCE_DIR}/CMake/PackageDepends")
list(APPEND MODULES_PACKAGE_DEPENDS_DIRS ${${MY_PROJECT_NAME}_MODULES_PACKAGE_DEPENDS_DIR})

#-----------------------------------------------------------------------------
# Get project version info
#-----------------------------------------------------------------------------

mitkFunctionGetVersion(${PROJECT_SOURCE_DIR} ${MY_PROJECT_NAME})

#-----------------------------------------------------------------------------
# Installation preparation
#
# These should be set before any MITK install macros are used
#-----------------------------------------------------------------------------

# on Mac OSX all CTK plugins get copied into every
# application bundle (.app directory) specified here
set(MACOSX_BUNDLE_NAMES)
if(APPLE)
  list(APPEND MACOSX_BUNDLE_NAMES ${MY_APP_NAME})
endif(APPLE)

#-----------------------------------------------------------------------------
# Set symbol visibility Flags
#-----------------------------------------------------------------------------

# MinGW does not export all symbols automatically, so no need to set flags
if(CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  # The MITK module build system does not yet support default hidden visibility
  set(VISIBILITY_CXX_FLAGS ) # "-fvisibility=hidden -fvisibility-inlines-hidden")
endif()

#-----------------------------------------------------------------------------
# Set coverage Flags
#-----------------------------------------------------------------------------

if(WITH_COVERAGE)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(coverage_flags "-g -fprofile-arcs -ftest-coverage -O0 -DNDEBUG")
    set(COVERAGE_CXX_FLAGS ${coverage_flags})
    set(COVERAGE_C_FLAGS ${coverage_flags})
  endif()
endif()

#-----------------------------------------------------------------------------
# Project C/CXX Flags
#-----------------------------------------------------------------------------

set(${MY_PROJECT_NAME}_C_FLAGS "${MITK_C_FLAGS} ${COVERAGE_C_FLAGS}")
set(${MY_PROJECT_NAME}_C_FLAGS_DEBUG ${MITK_C_FLAGS_DEBUG})
set(${MY_PROJECT_NAME}_C_FLAGS_RELEASE ${MITK_C_FLAGS_RELEASE})
set(${MY_PROJECT_NAME}_CXX_FLAGS "${MITK_CXX_FLAGS} ${VISIBILITY_CXX_FLAGS} ${COVERAGE_CXX_FLAGS}")
set(${MY_PROJECT_NAME}_CXX_FLAGS_DEBUG ${MITK_CXX_FLAGS_DEBUG})
set(${MY_PROJECT_NAME}_CXX_FLAGS_RELEASE ${MITK_CXX_FLAGS_RELEASE})

set(${MY_PROJECT_NAME}_EXE_LINKER_FLAGS ${MITK_EXE_LINKER_FLAGS})
set(${MY_PROJECT_NAME}_SHARED_LINKER_FLAGS ${MITK_SHARED_LINKER_FLAGS})
set(${MY_PROJECT_NAME}_MODULE_LINKER_FLAGS ${MITK_MODULE_LINKER_FLAGS})

# Add custom project flags here (e.g. for C++11 support)

if(CMAKE_COMPILER_IS_GNUCXX)
  # Get the gcc version. Some MITK CMake macros rely on the CMake
  # variable GCC_VERSION being set.
  mitkFunctionGetGccVersion(${CMAKE_CXX_COMPILER} GCC_VERSION)
  set(MITK_CXX_FEATURES cxx_generic_lambdas)
  set(${MY_PROJECT_NAME}_CXX_FLAGS -std=c++1y)
endif()

if(CMAKE_COMPILER_IS_MSVC)
    set(${MY_PROJECT_NAME}_CXX_FLAGS "${MY_PROJECT_NAME}_CXX_FLAGS /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /wd4251") # Suppress dll-interface warning
endif()
#-----------------------------------------------------------------------------
# Set C/CXX Flags
#-----------------------------------------------------------------------------

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${${MY_PROJECT_NAME}_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${${MY_PROJECT_NAME}_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${${MY_PROJECT_NAME}_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${MY_PROJECT_NAME}_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${${MY_PROJECT_NAME}_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${${MY_PROJECT_NAME}_CXX_FLAGS_RELEASE}")

set(CMAKE_EXE_LINKER_FLAGS ${${MY_PROJECT_NAME}_EXE_LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS ${${MY_PROJECT_NAME}_SHARED_LINKER_FLAGS})
set(CMAKE_MODULE_LINKER_FLAGS ${${MY_PROJECT_NAME}_MODULE_LINKER_FLAGS})

#-----------------------------------------------------------------------------
# Testing
#-----------------------------------------------------------------------------

if(BUILD_TESTING)
  enable_testing()
  include(CTest)
  mark_as_advanced(TCL_TCLSH DART_ROOT)

  # Setup file for setting custom ctest vars
  configure_file(
    CMake/CTestCustom.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake
    @ONLY
    )

  # Configuration for the CMake-generated test driver
  set(CMAKE_TESTDRIVER_EXTRA_INCLUDES "#include <stdexcept>")
  set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "
    try
      {")
  set(CMAKE_TESTDRIVER_AFTER_TESTMAIN " }
      catch( std::exception & excp )
        {
        fprintf(stderr,\"%s\\n\",excp.what());
        return EXIT_FAILURE;
        }
      catch( ... )
        {
        printf(\"Exception caught in the test driver\\n\");
        return EXIT_FAILURE;
        }
      ")
endif()

#-----------------------------------------------------------------------------
# ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR
#-----------------------------------------------------------------------------

# If ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR isn't defined, it means this project is
# *NOT* build using Superbuild. In that specific case, ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR
# should default to PROJECT_BINARY_DIR
if(NOT DEFINED ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR)
  set(${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR ${PROJECT_BINARY_DIR})
endif()

#-----------------------------------------------------------------------------
# Qt support
#-----------------------------------------------------------------------------

if(MITK_USE_Qt5)
  set(QT_QMAKE_EXECUTABLE ${MITK_QMAKE_EXECUTABLE})
    add_definitions(-DQWT_DLL)
endif()


# OpenCASCADE requirement
if (WIN32)
    add_definitions(-DWNT)
endif()

#-----------------------------------------------------------------------------
# MITK modules
#-----------------------------------------------------------------------------

# This project's directory holding module config files
set(${MY_PROJECT_NAME}_MODULES_CONF_DIR "${PROJECT_BINARY_DIR}/${MODULES_CONF_DIRNAME}")

# Append this projects's module config directory to the global list
# (This is used to get include directories for the <module_name>Exports.h files right)
list(APPEND MODULES_CONF_DIRS ${${MY_PROJECT_NAME}_MODULES_CONF_DIR})

# Clean the modulesConf directory. This ensures that modules are sorted
# according to their dependencies in the Modules/CMakeLists.txt file
file(GLOB _modules_conf_files ${${MY_PROJECT_NAME}_MODULES_CONF_DIR}/*.cmake)
if(_modules_conf_files)
  file(REMOVE ${_modules_conf_files})
endif()

add_subdirectory(Modules)

#-----------------------------------------------------------------------------
# CTK plugins
#-----------------------------------------------------------------------------

# The CMake code in this section *must* be in the top-level CMakeLists.txt file

macro(GetMyTargetLibraries all_target_libraries varname)
  set(re_ctkplugin "^uk_ac_[a-zA-Z0-9_]+$")
  set(_tmp_list)
  list(APPEND _tmp_list ${all_target_libraries})
  ctkMacroListFilter(_tmp_list re_ctkplugin OUTPUT_VARIABLE ${varname})
endmacro()

# Get infos about application directories and build options
include("${CMAKE_CURRENT_SOURCE_DIR}/Apps/Apps.cmake")
set(_apps_fullpath )
foreach(_app ${PROJECT_APPS})
  list(APPEND _apps_fullpath "${CMAKE_CURRENT_SOURCE_DIR}/${_app}")
endforeach()

include(${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Plugins.cmake)
ctkMacroSetupPlugins(${PROJECT_PLUGINS}
                     APPS ${_apps_fullpath}
                     BUILD_OPTION_PREFIX ${MY_PROJECT_NAME}_
                     BUILD_ALL ${${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS})

#-----------------------------------------------------------------------------
# Add subdirectories
#-----------------------------------------------------------------------------

add_subdirectory(Apps)

# #-----------------------------------------------------------------------------
# # Installation
# #-----------------------------------------------------------------------------

# # set MITK cpack variables
# include(mitkSetupCPack)

# # Customize CPack variables for this project
# include(CPackSetup)

# list(APPEND CPACK_CREATE_DESKTOP_LINKS "${MY_APP_NAME}")

# include(${CMAKE_CURRENT_SOURCE_DIR}/Apps/CRIMSON/CPackOptions.cmake)
# set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/${MY_PROJECT_NAME}CPackOptions.cmake")
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Apps/CRIMSON/CPackConfig.cmake.in
               # ${CPACK_PROJECT_CONFIG_FILE} @ONLY)

# # include CPack model once all variables are set
# include(CPack)

# # Additional installation rules
# include(mitkInstallRules)

#-----------------------------------------------------------------------------
# Last configuration steps
#-----------------------------------------------------------------------------
