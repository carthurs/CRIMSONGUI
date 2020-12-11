message("Entering ./NonSuperBuild.cmake")
# This file is used for building / configuring the Crimson application project, 
# It looks like the main build script runs the SuperBuild script first, then this one.
#       
# Note that this script is only responsible for building CRIMSON and its included plugins.

#-----------------------------------------------------------------------------
# Prerequesites
#-----------------------------------------------------------------------------

# - At first, I figured ExternalProject_Add would make this find_package call just exit immediately or run CMakeExternals/MITK.cmake run
# - But I think a package is not necessarily a project; I think it goes ExternalProject, THEN package
#
# This link describes what happens.
# https://cmake.org/cmake/help/latest/command/find_package.html#full-signature-and-config-mode
# - There is no findMITK.cmake file, so it falls back to Config mode.
# - It looks for a variable named [PackageName]_DIR, in this case MITK_DIR, and in there it looks for [PackageName]Config.cmake
# - It finds and executes C:\cr\MITK-superbuild\MITK-build\MITKConfig.cmake (or wherever it is in your build directory)
# - Which is a configured version of <root of MITK git repository>/MITKConfig.cmake.in
#     (which in my case is C:\cr\MITK\MITKConfig.cmake.in)
#   What configures this file? 
#   - C:\cr\MITK\CMakeLists.txt
#     - This CMakeLists.txt gets ran at *build* time, MITK isn't even git cloned until the main project gets built.
#     - I would guess that this happens in the MITK VS Project, and that's what ExternalProject_Add is for.

# uncomment this to see everything that happens to CMAKE_MODULE_PATH
# variable_watch(CMAKE_MODULE_PATH)
message("Attempting to find MITK... CMAKE_MODULE_PATH is " ${CMAKE_MODULE_PATH} " MITK_DIR is " ${MITK_DIR})
find_package(MITK 2016.11.0 REQUIRED)
message("After find_package, CMAKE_MODULE_PATH is " ${CMAKE_MODULE_PATH})

if(COMMAND mitkFunctionCheckMitkCompatibility)
  mitkFunctionCheckMitkCompatibility(VERSIONS MITK_VERSION_PLUGIN_SYSTEM 1 REQUIRED)
else()
  message(SEND_ERROR "Your MITK version is too old. Please use Git hash b86bf28 or newer")
endif()

link_directories(${MITK_LINK_DIRECTORIES})

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

#[AJM]  this seems unnecessary, it appears that find_package has already added this directory, that specific
#       MITK directory shows twice in CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${MITK_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH} )
message("Added  {MITK_SOURCE_DIR}/CMake=" ${MITK_SOURCE_DIR}/CMake " CMAKE_MODULE_PATH is now " ${CMAKE_MODULE_PATH})

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
set(${MY_PROJECT_NAME}_VERSION_PATCH "01")   
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

# note that this macro is called by ctkMacroSetupPlugins
# https://github.com/commontk/CTK/blob/b9393da54b69ac64d1e8cdceae6bd665cfc96f14/CMake/ctkMacroSetupPlugins.cmake#L147
#   https://github.com/commontk/CTK/blob/613b8207b5f028b7e0617fdd889e675bb027eeff/CMake/ctkMacroTargetLibraries.cmake#L230
#     https://github.com/commontk/CTK/blob/613b8207b5f028b7e0617fdd889e675bb027eeff/CMake/ctkMacroTargetLibraries.cmake#L213
#
# Remember that because we're working with macros, scope is shared
macro(GetMyTargetLibraries all_target_libraries varname)
  set(re_ctkplugin "^uk_ac_[a-zA-Z0-9_]+$")
  set(_tmp_list)
  list(APPEND _tmp_list ${all_target_libraries})

  # [AJM] any collected dlls that do not match the regex specified above will be removed from the list,
  #       meaning only dlls that match the regex pattern will be in _tmp_list after the command below runs.
  ctkMacroListFilter(_tmp_list re_ctkplugin OUTPUT_VARIABLE ${varname})
endmacro()

# Get infos about application directories and build options
# Note: This included file just more or less sets the PROJECT_APPS variable.
include("${CMAKE_CURRENT_SOURCE_DIR}/Apps/Apps.cmake")
set(_apps_fullpath )

# Note: In our case PROJECT_APPS will only have one entry
foreach(_app ${PROJECT_APPS})
  list(APPEND _apps_fullpath "${CMAKE_CURRENT_SOURCE_DIR}/${_app}")
endforeach()

# Note: this include just sets the PROJECT_PLUGINs variable
include(${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Plugins.cmake)
ctkMacroSetupPlugins(${PROJECT_PLUGINS}
                     APPS ${_apps_fullpath}
                     BUILD_OPTION_PREFIX ${MY_PROJECT_NAME}_
                     BUILD_ALL ${${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS})

#-----------------------------------------------------------------------------
# Add subdirectories
#-----------------------------------------------------------------------------

message("./CMakeLists: Including Apps directory")
add_subdirectory(Apps)

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

# set MITK cpack variables
include(mitkSetupCPack)

# Customize CPack variables for this project
include("CMake/NonSuperbuildCPackSetup.cmake")

list(APPEND CPACK_CREATE_DESKTOP_LINKS "${MY_APP_NAME}")

include(${CMAKE_CURRENT_SOURCE_DIR}/Apps/CRIMSON/CPackOptions.cmake)
set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/${MY_PROJECT_NAME}CPackOptions.cmake")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Apps/CRIMSON/CPackConfig.cmake.in ${CPACK_PROJECT_CONFIG_FILE} @ONLY)

# include CPack model once all variables are set
include(CPack)

# Additional installation rules
include(mitkInstallRules)

message("End of ./NonSuperBuild.cmake")