message("Entering ./SuperBuild.cmake")
# This file is used for collecting and compiling CRIMSON's dependencies. 
# It will also run NonSuperBuild.cmake to compile crimson itself.

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

include(MacroEmptyExternalProject)

#-----------------------------------------------------------------------------
# Convenient macro allowing to download a file
#-----------------------------------------------------------------------------

if(NOT MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL)
  set(MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL http://mitk.org/download/thirdparty)
endif()

macro(downloadFile url dest)
  file(DOWNLOAD ${url} ${dest} STATUS status)
  list(GET status 0 error_code)
  list(GET status 1 error_msg)
  if(error_code)
    message(FATAL_ERROR "error: Failed to download ${url} - ${error_msg}")
  endif()
endmacro()

# We need a proper patch program. On Linux and MacOS, we assume
# that "patch" is available. On Windows, we download patch.exe
# if not patch program is found.
find_program(PATCH_COMMAND patch)
if((NOT PATCH_COMMAND OR NOT EXISTS ${PATCH_COMMAND}) AND WIN32)
  downloadFile(${MITK_THIRDPARTY_DOWNLOAD_PREFIX_URL}/patch.exe
               ${CMAKE_CURRENT_BINARY_DIR}/patch.exe)
  find_program(PATCH_COMMAND patch ${CMAKE_CURRENT_BINARY_DIR})
endif()
if(NOT PATCH_COMMAND)
  message(FATAL_ERROR "No patch program found.")
endif()

#-----------------------------------------------------------------------------
# ExternalProjects
#-----------------------------------------------------------------------------

set(PACKAGE_FLOWSOLVER OFF CACHE BOOL "Download the flowsolver as part of the build process (not currently supported)")

set(external_projects
  freetype
  TBB
  OCC
  WM5
  QtPropertyBrowser
  presolver
  GSL

  )

if(PACKAGE_FLOWSOLVER)
  list(APPEND external_projects flowsolver)
endif()

# [AJM] why is this excluded from debug builds?
if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  list(APPEND external_projects PythonModules parse)
endif()
  

set(EXTERNAL_MITK_DIR "${MITK_DIR}" CACHE PATH "Path to MITK build directory (if you want to re-use a prebuilt directory for speed reasons). Example: C:/cr/MITK-superbuild/MITK-build")
mark_as_advanced(EXTERNAL_MITK_DIR)
if(EXTERNAL_MITK_DIR)
  set(MITK_DIR ${EXTERNAL_MITK_DIR})
endif()

# [AJM]
set(EXTERNAL_OCC_DIR "" CACHE PATH "Path to OCC directory (if you want to re-use a prebuilt directory for speed reasons), example: C:/cr/CMakeExternals/Install/OCC_src/ directory")
mark_as_advanced(EXTERNAL_OCC_DIR)
if(EXTERNAL_OCC_DIR)
  set(OCC_DIR ${EXTERNAL_OCC_DIR})
endif()

# Look for git early on, if needed
# [AJM] this does not necessarily mean that git is ready to commit, though
#   some parts of the build script (unfortunately) use stash, which will fail
#   if you haven't set your name and e-mail.
if(NOT MITK_DIR AND MITK_USE_CTK AND NOT CTK_DIR)
  find_package(Git REQUIRED)
endif()

#-----------------------------------------------------------------------------
# External project settings
#-----------------------------------------------------------------------------

include(ExternalProject)

set(ep_base "${CMAKE_BINARY_DIR}/CMakeExternals")
set_property(DIRECTORY PROPERTY EP_BASE ${ep_base})

set(ep_install_dir "${CMAKE_BINARY_DIR}/CMakeExternals/Install")
set(ep_suffix "-cmake")
set(ep_build_shared_libs ON)
set(ep_build_testing OFF)

# Compute -G arg for configuring external projects with the same CMake generator:
if(CMAKE_EXTRA_GENERATOR)
  set(gen "${CMAKE_EXTRA_GENERATOR} - ${CMAKE_GENERATOR}")
else()
  set(gen "${CMAKE_GENERATOR}")
endif()

# Use this value where semi-colons are needed in ep_add args:
set(sep "^^")

##

if(MSVC_VERSION)
  list(APPEND CMAKE_C_FLAGS "/bigobj" "/MP")
  list(APPEND CMAKE_CXX_FLAGS "/bigobj" "/MP")
else()
  # GCC?
  list(APPEND CMAKE_CXX_FLAGS "-std=c++1y")
endif()

set(ep_common_args
  -DBUILD_TESTING:BOOL=${ep_build_testing}
  -DCMAKE_INSTALL_PREFIX:PATH=${ep_install_dir}
  -DBUILD_SHARED_LIBS:BOOL=${ep_build_shared_libs}
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
  -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
  # debug flags
  -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
  -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
  # release flags
  -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
  -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
  # relwithdebinfo
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
  -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
  # link flags
  -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
  -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
  -DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
)

# Include the .cmake file for each external project
foreach(p MITK ${external_projects})
  include(CMakeExternals/${p}.cmake)
endforeach()

#-----------------------------------------------------------------------------
# Set superbuild boolean args
#-----------------------------------------------------------------------------

set(my_cmake_boolean_args
  WITH_COVERAGE
  BUILD_TESTING
  ${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS
  )

#-----------------------------------------------------------------------------
# Create the final variable containing superbuild boolean args
#-----------------------------------------------------------------------------

set(my_superbuild_boolean_args)
foreach(my_cmake_arg ${my_cmake_boolean_args})
  list(APPEND my_superbuild_boolean_args -D${my_cmake_arg}:BOOL=${${my_cmake_arg}})
endforeach()

#-----------------------------------------------------------------------------
# CRIMSON Utilities
#-----------------------------------------------------------------------------

set(proj ${MY_PROJECT_NAME}-Utilities)
ExternalProject_Add(${proj} # where proj is CRIMSON-Utilities
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    # Mandatory dependencies
    ${MITK_DEPENDS}
    # Optional dependencies
)

#-----------------------------------------------------------------------------
# Additional Project CXX/C Flags
#-----------------------------------------------------------------------------

set(${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS "" CACHE STRING "Additional C Flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_RELEASE "" CACHE STRING "Additional Release C Flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_DEBUG "" CACHE STRING "Additional Debug C Flags for ${MY_PROJECT_NAME}")
mark_as_advanced(${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS ${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_DEBUG ${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_RELEASE)

set(${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS "" CACHE STRING "Additional CXX Flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_RELEASE "" CACHE STRING "Additional Release CXX Flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_DEBUG "" CACHE STRING "Additional Debug CXX Flags for ${MY_PROJECT_NAME}")
mark_as_advanced(${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS ${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_DEBUG ${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_RELEASE)

set(${MY_PROJECT_NAME}_ADDITIONAL_EXE_LINKER_FLAGS "" CACHE STRING "Additional exe linker flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_SHARED_LINKER_FLAGS "" CACHE STRING "Additional shared linker flags for ${MY_PROJECT_NAME}")
set(${MY_PROJECT_NAME}_ADDITIONAL_MODULE_LINKER_FLAGS "" CACHE STRING "Additional module linker flags for ${MY_PROJECT_NAME}")
mark_as_advanced(${MY_PROJECT_NAME}_ADDITIONAL_EXE_LINKER_FLAGS ${MY_PROJECT_NAME}_ADDITIONAL_SHARED_LINKER_FLAGS ${MY_PROJECT_NAME}_ADDITIONAL_MODULE_LINKER_FLAGS)

#-----------------------------------------------------------------------------
# CRIMSON-Configure
#-----------------------------------------------------------------------------
# This section is responsible for creating the C:\cb\CRIMSON-build directory,
# and its contents, including CRIMSON.sln in that directory.

message("Running CRIMSON-Configure (creating CRIMSON-build directory)")

message("OCC_DIR is " ${OCC_DIR})
message("QtPropertyBrowser_DIR is " ${QtPropertyBrowser_DIR})
message("CMAKE_MODULE_PATH  " ${CMAKE_MODULE_PATH})

set(proj ${MY_PROJECT_NAME}-Configure)

# [AJM] it looks like this is reconfiguring a new project, and it attempts to ... manually transfer all of the custom settings.
#       I am not sure why it's doing that, seems like there must be a better way than this.
#       Why exactly can't this just re-use the CMAKE_ARGS that we already set? Can it? 
#       - Do any of the other projects successfully re-use CMAKE_ARGS?
#       - Can I just do something like CMAKE_ARGS ${CMAKE_ARGS}?
#
#       NOTE: This project "Recycles" ./CMakeLists.txt...
#             I wonder if this is well defined behavior, I am not sure if this is a good idea...
ExternalProject_Add(${proj} #where proj is CRIMSON-Configure
  DOWNLOAD_COMMAND ""
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    # --------------- Build options ----------------
    -DBUILD_TESTING:BOOL=${ep_build_testing}
    -DCMAKE_INSTALL_PREFIX:PATH=${ep_install_dir}
    -DBUILD_SHARED_LIBS:BOOL=${ep_build_shared_libs}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    # --------------- Compile options ----------------
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} ${${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS}"
    "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS}"
    # debug flags
    "-DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG} ${${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_DEBUG}"
    "-DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG} ${${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_DEBUG}"
    # release flags
    "-DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE} ${${MY_PROJECT_NAME}_ADDITIONAL_CXX_FLAGS_RELEASE}"
    "-DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE} ${${MY_PROJECT_NAME}_ADDITIONAL_C_FLAGS_RELEASE}"
    # relwithdebinfo
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
    -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
    # link flags
    "-DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS} ${${MY_PROJECT_NAME}_ADDITIONAL_EXE_LINKER_FLAGS}"
    "-DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS} ${${MY_PROJECT_NAME}_ADDITIONAL_SHARED_LINKER_FLAGS}"
    "-DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS} ${${MY_PROJECT_NAME}_ADDITIONAL_MODULE_LINKER_FLAGS}"
    # ------------- Boolean build options --------------
    ${my_superbuild_boolean_args}

    #[AJM]  Deliberately always OFF (even if it's ON for the main script); if SuperBuild isn't OFF for this project, then it'll just re-run the SuperBuild.cmake 
    #       script instead of building CRIMSON itself
    -D${MY_PROJECT_NAME}_USE_SUPERBUILD:BOOL=OFF
    -D${MY_PROJECT_NAME}_CONFIGURED_VIA_SUPERBUILD:BOOL=ON
    -DCTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
    # ----------------- Miscellaneous ---------------
    -D${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR:PATH=${PROJECT_BINARY_DIR}
    -DQt5_DIR:PATH=${Qt5_DIR}
    -DCMAKE_PREFIX_PATH:PATH=${Qt5_DIR}
    -DMITK_DIR:PATH=${MITK_DIR}
    -DITK_DIR:PATH=${ITK_DIR}
    -DVTK_DIR:PATH=${VTK_DIR}
    -DOCC_DIR:PATH=${OCC_DIR}
    -DWM5_ROOT_DIR:PATH=${WM5_DIR}
    -DQtPropertyBrowser_DIR:PATH=${QtPropertyBrowser_DIR}
    -DPRESOLVER_EXECUTABLE:FILEPATH=${presolver_executable}
    -DGSL_INCLUDE_DIR:PATH=${GSL_INCLUDE_DIR}
    -DCRIMSON_MESHING_KERNEL:STRING=${CRIMSON_MESHING_KERNEL}
    
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} # Since we're running in ./, this will cause cmake to re-run ./CMakeLists.txt!
  BINARY_DIR ${CMAKE_BINARY_DIR}/${MY_PROJECT_NAME}-build
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    ${MY_PROJECT_NAME}-Utilities
    ${external_projects}
    )


#-----------------------------------------------------------------------------
# Project
#-----------------------------------------------------------------------------

if(CMAKE_GENERATOR MATCHES ".*Makefiles.*")
  set(_build_cmd "$(MAKE)")
else()
  set(_build_cmd ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/${MY_PROJECT_NAME}-build --config ${CMAKE_CFG_INTDIR})
endif()

# The variable SUPERBUILD_EXCLUDE_${MY_PROJECT_NAME}BUILD_TARGET should be set when submitting to a dashboard
if(NOT DEFINED SUPERBUILD_EXCLUDE_${MY_PROJECT_NAME}BUILD_TARGET OR NOT SUPERBUILD_EXCLUDE_${MY_PROJECT_NAME}BUILD_TARGET)
  set(_target_all_option "ALL")
else()
  set(_target_all_option "")
endif()

add_custom_target(${MY_PROJECT_NAME}-build ${_target_all_option}
  COMMAND ${_build_cmd}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MY_PROJECT_NAME}-build
  DEPENDS ${MY_PROJECT_NAME}-Configure
  )

#-----------------------------------------------------------------------------
# Custom target allowing to drive the build of the project itself
#-----------------------------------------------------------------------------

add_custom_target(${MY_PROJECT_NAME}
  COMMAND ${_build_cmd}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MY_PROJECT_NAME}-build
)