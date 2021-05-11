#-----------------------------------------------------------------------------
# MITK
#-----------------------------------------------------------------------------

set(MITK_DEPENDS)
set(proj_DEPENDENCIES)
set(proj MITK)

if(NOT MITK_DIR)

  #-----------------------------------------------------------------------------
  # Create CMake options to customize the MITK build
  #-----------------------------------------------------------------------------

  option(MITK_USE_SUPERBUILD "Use superbuild for MITK" ON)
  option(MITK_USE_BLUEBERRY "Build the BlueBerry platform in MITK" ON)
  option(MITK_BUILD_TUTORIAL "Build the MITK tutorial" OFF)
  option(MITK_BUILD_ALL_PLUGINS "Build all MITK plugins" OFF)
  option(MITK_BUILD_TESTING "Build the MITK unit tests" OFF)
  option(MITK_USE_CTK "Use CTK in MITK" ${MITK_USE_BLUEBERRY})
  option(MITK_USE_DCMTK "Use DCMTK in MITK" ON)
  option(MITK_USE_Qt5 "Use Qt 5 library in MITK" ON)
  option(MITK_USE_Qt5_WebEngine "Use Qt 5 WebEngine library" ON)
  option(MITK_USE_Boost "Use the Boost library in MITK" ON)
  option(MITK_USE_OpenCV "Use Intel's OpenCV library" OFF)
  option(MITK_USE_Python "Enable Python wrapping in MITK" ON)
  
  if(MITK_USE_BLUEBERRY AND NOT MITK_USE_CTK)
    message("Forcing MITK_USE_CTK to ON because of MITK_USE_BLUEBERRY")
    set(MITK_USE_CTK ON CACHE BOOL "Use CTK in MITK" FORCE)
  endif()

  if(MITK_USE_CTK AND NOT MITK_USE_Qt5)
    message("Forcing MITK_USE_Qt5 to ON because of MITK_USE_CTK")
    set(MITK_USE_QT ON CACHE BOOL "Use Qt 5 library in MITK" FORCE)
  endif()
  
  set(MITK_USE_CableSwig ${MITK_USE_Python})
  set(MITK_USE_GDCM 1)
  set(MITK_USE_ITK 1)
  set(MITK_USE_VTK 1)
      
  mark_as_advanced(MITK_USE_SUPERBUILD
                   MITK_BUILD_ALL_PLUGINS
                   MITK_BUILD_TESTING
                   )

  set(mitk_cmake_boolean_args
    MITK_USE_SUPERBUILD
    MITK_USE_BLUEBERRY
    MITK_BUILD_TUTORIAL
    MITK_BUILD_ALL_PLUGINS
    MITK_USE_CTK
    MITK_USE_DCMTK
    MITK_USE_Qt5
    MITK_USE_Qt5_WebEngine
    MITK_USE_Boost
    MITK_USE_OpenCV
    MITK_USE_Python
   )
   
  set(DESIRED_QT_VERSION 5)

  if(MITK_USE_Qt5)
    set(MITK_QT5_MINIMUM_VERSION 5.6.0)
    set(MITK_QT5_COMPONENTS Concurrent OpenGL PrintSupport Script Sql Svg Widgets Xml XmlPatterns UiTools Help LinguistTools)
    if(MITK_USE_Qt5_WebEngine)
      set(MITK_QT5_COMPONENTS ${MITK_QT5_COMPONENTS} WebEngineWidgets)
    endif()
    if(APPLE)
      set(MITK_QT5_COMPONENTS ${MITK_QT5_COMPONENTS} DBus)
    endif()
    find_package(Qt5 ${MITK_QT5_MINIMUM_VERSION} COMPONENTS ${MITK_QT5_COMPONENTS} REQUIRED)
    if(Qt5_DIR)
      get_filename_component(_Qt5_DIR "${Qt5_DIR}/../../../" ABSOLUTE)
      list(FIND CMAKE_PREFIX_PATH "${_Qt5_DIR}" _result)
      if(_result LESS 0)
        set(CMAKE_PREFIX_PATH "${_Qt5_DIR};${CMAKE_PREFIX_PATH}" CACHE PATH "" FORCE)
      endif()
    endif()
  elseif(MITK_USE_Qt5_WebEngine)
    set(MITK_USE_Qt5_WebEngine OFF)
  endif()
  
  set(additional_mitk_cmakevars
    -DDESIRED_QT_VERSION:STRING=${DESIRED_QT_VERSION}
  )
  
  # Configure the set of default pixel types
  set(MITK_ACCESSBYITK_INTEGRAL_PIXEL_TYPES
      "int, unsigned int, short, unsigned short, char, unsigned char"
      CACHE STRING "List of integral pixel types used in AccessByItk and InstantiateAccessFunction macros")

  set(MITK_ACCESSBYITK_FLOATING_PIXEL_TYPES
      "double, float"
      CACHE STRING "List of floating pixel types used in AccessByItk and InstantiateAccessFunction macros")

  set(MITK_ACCESSBYITK_COMPOSITE_PIXEL_TYPES
      "itk::RGBPixel<unsigned char>, itk::RGBAPixel<unsigned char>"
      CACHE STRING "List of composite pixel types used in AccessByItk and InstantiateAccessFunction macros")

  set(MITK_ACCESSBYITK_DIMENSIONS
      "2,3"
      CACHE STRING "List of dimensions used in AccessByItk and InstantiateAccessFunction macros")

  foreach(_arg MITK_ACCESSBYITK_INTEGRAL_PIXEL_TYPES MITK_ACCESSBYITK_FLOATING_PIXEL_TYPES
               MITK_ACCESSBYITK_COMPOSITE_PIXEL_TYPES MITK_ACCESSBYITK_DIMENSIONS)
    mark_as_advanced(${_arg})
    list(APPEND additional_mitk_cmakevars "-D${_arg}:STRING=${${_arg}}")
  endforeach()
   
  #-----------------------------------------------------------------------------
  # Create options to inject pre-build dependencies
  #-----------------------------------------------------------------------------
  
  # [AJM] NOTE: The foreach control variable has different scoping rules,
  #             https://cmake.org/pipermail/cmake/2005-June/006729.html
  #             I don't think this is an actual variable of its own, it just references... existing variables?
  #             So there is no name collision / data overwriting issue with using proj here as a loop variable.
  foreach(proj CTK DCMTK GDCM VTK ITK OpenCV CableSwig)
    if(MITK_USE_${proj})
      set(MITK_${proj}_DIR "${${proj}_DIR}" CACHE PATH "Path to ${proj} build directory")
      mark_as_advanced(MITK_${proj}_DIR)
      if(MITK_${proj}_DIR)
        list(APPEND additional_mitk_cmakevars "-D${proj}_DIR:PATH=${MITK_${proj}_DIR}")
      endif()
    endif()
  endforeach()

  if(MITK_USE_Boost)
    set(MITK_BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Path to Boost directory")
    mark_as_advanced(MITK_BOOST_ROOT)
    if(MITK_BOOST_ROOT)
      list(APPEND additional_mitk_cmakevars "-DBOOST_ROOT:PATH=${MITK_BOOST_ROOT}")
    endif()
    list(APPEND additional_mitk_cmakevars "-DMITK_USE_Boost_LIBRARIES:STRING=serialization^^system^^date_time^^graph^^thread^^regex")
  endif()
  
  set(MITK_SOURCE_DIR "" CACHE PATH "MITK source code location. If empty, MITK will be cloned from MITK_GIT_REPOSITORY") #use this to test local changes before pushing to github

  # [AJM] Note: this is a custom version of MITK for CRIMSON's purposes.
  set(MITK_GIT_REPOSITORY "https://github.com/carthurs/MITK.git" CACHE STRING "The git repository for cloning MITK")

  # [AJM] Don't just pull whatever the latest is on the development branch, that will make it impossible to compile older versions of the program later on.
  set(MITK_GIT_TAG "CRIMSON_2021.05" CACHE STRING "The git tag/hash to be used when cloning from MITK_GIT_REPOSITORY")
  mark_as_advanced(MITK_SOURCE_DIR MITK_GIT_REPOSITORY MITK_GIT_TAG)
  
  #-----------------------------------------------------------------------------
  # Create the final variable containing superbuild boolean args
  #-----------------------------------------------------------------------------

  set(mitk_boolean_args)
  foreach(mitk_cmake_arg ${mitk_cmake_boolean_args})
    list(APPEND mitk_boolean_args -D${mitk_cmake_arg}:BOOL=${${mitk_cmake_arg}})
  endforeach()

  #-----------------------------------------------------------------------------
  # Additional MITK CMake variables
  #-----------------------------------------------------------------------------

  if(MITK_USE_Qt5)
    find_package(Qt5 REQUIRED COMPONENTS Core)
    list(APPEND additional_mitk_cmakevars "-DQt5_DIR:PATH=${Qt5_DIR}" "-DCMAKE_PREFIX_PATH:PATH=${Qt5_DIR}")
  endif()
  
  if(MITK_USE_CTK)
    list(APPEND additional_mitk_cmakevars "-DGIT_EXECUTABLE:FILEPATH=${GIT_EXECUTABLE}")
  endif()
  
  if(MITK_INITIAL_CACHE_FILE)
    list(APPEND additional_mitk_cmakevars "-DMITK_INITIAL_CACHE_FILE:INTERNAL=${MITK_INITIAL_CACHE_FILE}")
  endif()

  if(MITK_USE_SUPERBUILD)
    set(MITK_BINARY_DIR ${proj}-superbuild)
  else()
    set(MITK_BINARY_DIR ${proj}-build)
  endif()
  
  set(proj_DEPENDENCIES)
  set(MITK_DEPENDS ${proj})
  
  # Configure the MITK souce code location
  
  if(NOT MITK_SOURCE_DIR)
    set(mitk_source_location
        SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
        GIT_REPOSITORY ${MITK_GIT_REPOSITORY}
        GIT_TAG ${MITK_GIT_TAG}
        )
  else()
    set(mitk_source_location
        SOURCE_DIR ${MITK_SOURCE_DIR}
       )
  endif()

  # MITK Python only supports release builds, so we need to set the CMAKE_BUILD_TYPE accordingly
  set(mitkBuildType ${CMAKE_BUILD_TYPE})
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug builds are not supported for MITK Python, using RelWithDebInfo instead.")

    set(mitkBuildType "RelWithDebInfo")
  endif()
  

  ExternalProject_Add(${proj}
    ${mitk_source_location}
    BINARY_DIR ${MITK_BINARY_DIR}
    # where ep_suffix is -cmake, set in SuperBuild.cmake; I think ep stands for "External Project"
    PREFIX ${proj}${ep_suffix}
    INSTALL_COMMAND ""
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      ${ep_common_args}
      ${mitk_boolean_args}
      ${additional_mitk_cmakevars}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_TESTING:BOOL=${MITK_BUILD_TESTING}
      -DCMAKE_BUILD_TYPE:STRING=${mitkBuildType}
    CMAKE_CACHE_ARGS
      ${ep_common_cache_args}
    CMAKE_CACHE_DEFAULT_ARGS
      ${ep_common_cache_default_args}
    DEPENDS
      ${proj_DEPENDENCIES}
    )
  
  if(MITK_USE_SUPERBUILD)
    set(MITK_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MITK_BINARY_DIR}/MITK-build")
  else()
    set(MITK_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MITK_BINARY_DIR}")
  endif()

else()

  # The project is provided using MITK_DIR, nevertheless since other 
  # projects may depend on MITK, let's add an 'empty' one
  MacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
  
  # Further, do some sanity checks in the case of a pre-built MITK
  set(my_itk_dir ${ITK_DIR})
  set(my_vtk_dir ${VTK_DIR})
  set(my_qmake_executable ${QT_QMAKE_EXECUTABLE})

  find_package(MITK REQUIRED)
  
  if(my_itk_dir AND ITK_DIR)
    if(NOT my_itk_dir STREQUAL ${ITK_DIR})
      message(FATAL_ERROR "ITK packages do not match:\n   ${MY_PROJECT_NAME}: ${my_itk_dir}\n  MITK: ${ITK_DIR}")
    endif()
  endif()
  
  if(my_vtk_dir AND VTK_DIR)
    if(NOT my_vtk_dir STREQUAL ${VTK_DIR})
      message(FATAL_ERROR "VTK packages do not match:\n   ${MY_PROJECT_NAME}: ${my_vtk_dir}\n  MITK: ${VTK_DIR}")
    endif()
  endif()
  
  if(my_qmake_executable AND MITK_QMAKE_EXECUTABLE)
    if(NOT my_qmake_executable STREQUAL ${MITK_QMAKE_EXECUTABLE})
      message(FATAL_ERROR "Qt qmake does not match:\n   ${MY_PROJECT_NAME}: ${my_qmake_executable}\n  MITK: ${MITK_QMAKE_EXECUTABLE}")
    endif()
  endif()
    
endif()
