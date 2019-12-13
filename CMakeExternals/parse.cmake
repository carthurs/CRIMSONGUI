#-----------------------------------------------------------------------------
# parse
#-----------------------------------------------------------------------------
if( MITK_USE_Python AND NOT MITK_USE_SYSTEM_PYTHON )
  # Sanity checks
  if(DEFINED parse_DIR AND NOT EXISTS ${parse_DIR})
    message(FATAL_ERROR "parse_DIR variable is defined but corresponds to non-existing directory")
  endif()

  if( NOT DEFINED parse_DIR )
    set(proj parse)
    set(${proj}_DEPENDENCIES MITK)
    set(parse_DEPENDS ${proj})

    set(_parse_build_step ${CMAKE_CURRENT_SOURCE_DIR}/CMake/crimsonFunctionExternalPythonBuildStep.cmake)

    set(_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/parse)
    
    # install step
    set(_install_step ${CMAKE_BINARY_DIR}/CMakeExternals/tmp/${proj}_install_step.cmake)

    # escape spaces
    if(UNIX)
      STRING(REPLACE " " "\ " _install_step ${_install_step})
    endif()
    
    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL https://pypi.python.org/packages/source/p/parse/parse-1.6.6.tar.gz
      URL_MD5 11bc8c60a30fe52db4ac9a827653d0ca
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ""
      BUILD_COMMAND   ""
      INSTALL_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DMITK_DIR:PATH=${MITK_DIR} -P ${_install_step}

      DEPENDS
        ${${proj}_DEPENDENCIES}
    )
    
    ExternalProject_Get_Property(${proj} source_dir)

    file(WRITE ${_install_step}
       "include(\"${_parse_build_step}\")
        crimsonFunctionExternalPythonBuildStep(${proj} install \"${source_dir}\" setup.py install)
       ")

    set(parse_DIR ${MITK_PYTHON_SITE_DIR}/parse)
    install(SCRIPT ${_install_step})

  else()
    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
  endif()
endif()

