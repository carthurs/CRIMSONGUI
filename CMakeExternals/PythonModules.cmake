#-----------------------------------------------------------------------------
# PythonModules
#-----------------------------------------------------------------------------
if( MITK_USE_Python AND NOT MITK_USE_SYSTEM_PYTHON )
  # Sanity checks
  if(DEFINED PythonModules_DIR AND NOT EXISTS ${PythonModules_DIR})
    message(FATAL_ERROR "PythonModules_DIR variable is defined but corresponds to non-existing directory")
  endif()

  if( NOT DEFINED PythonModules_DIR )
    set(proj PythonModules)
    set(${proj}_DEPENDENCIES MITK presolver flowsolver)
    set(PythonModules_DEPENDS ${proj})

    set(_PythonModules_build_step ${CMAKE_CURRENT_SOURCE_DIR}/CMake/crimsonFunctionExternalPythonBuildStep.cmake)
	

    # install step
    set(_install_step ${CMAKE_BINARY_DIR}/CMakeExternals/tmp/${proj}_install_step.cmake)
    file(WRITE ${_install_step}
       "include(\"${_PythonModules_build_step}\")
        crimsonFunctionExternalPythonBuildStep(${proj} install \"\${source_dir}\" setup.py install --force)
       ")
      
    # build step
    set(_build_step ${CMAKE_BINARY_DIR}/CMakeExternals/tmp/${proj}_build_step.cmake)
    file(WRITE ${_build_step}
       "include(\"${_PythonModules_build_step}\")
        crimsonFunctionExternalPythonBuildStep(${proj} build \"\${source_dir}\" setup.py build --force)
       ")

    # escape spaces
    if(UNIX)
      STRING(REPLACE " " "\ " _install_step ${_install_step})
      STRING(REPLACE " " "\ " _build_step ${_build_step})
    endif()

    set(_configure_step ${CMAKE_BINARY_DIR}/CMakeExternals/tmp/${proj}_configure_step.cmake)
    file(WRITE ${_configure_step}
       "file(COPY ${flowsolver_folder} DESTINATION \"\${source_dir}/CRIMSONSolver/SolverStudies/\")
        file(COPY ${presolver_executable} DESTINATION \"\${source_dir}/CRIMSONSolver/SolverStudies/\")
        get_filename_component(presolver_executable_filename ${presolver_executable} NAME)
        configure_file(\"\${source_dir}/CRIMSONSolver/SolverStudies/PresolverExecutableName.py.in\" \"\${source_dir}/CRIMSONSolver/SolverStudies/PresolverExecutableName.py\")
        configure_file(\"\${source_dir}/setup.py.in\" \"\${source_dir}/setup.py\")
       ")

    set(python_lib_folder "Lib/")
    if (UNIX)
        set(python_lib_folder "lib/python2.7/")
    endif()
	
	set(PY_SOURCE_DIR "" CACHE PATH "source code location. If empty, will be cloned from GIT_REPOSITORY")
    set(PY_GIT_REPOSITORY "https://github.com/rkhlebnikov/CRIMSONPythonModules.git" CACHE STRING "The git repository for cloning MITK")
    set(PY_GIT_TAG "origin/open_source" CACHE STRING "The git tag/hash to be used when cloning from MITK_GIT_REPOSITORY")

  
  if(NOT PY_SOURCE_DIR)
    set(py_source_location
        SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
        GIT_REPOSITORY ${PY_GIT_REPOSITORY}
        GIT_TAG ${PY_GIT_TAG}
        )
  else()
    set(py_source_location
        SOURCE_DIR ${PY_SOURCE_DIR}
       )
  endif()

    ExternalProject_Add(${proj}
	  ${py_source_location}
      LIST_SEPARATOR ${sep}
      #GIT_REPOSITORY https://github.com/rkhlebnikov/CRIMSONPythonModules.git
      BUILD_IN_SOURCE 1
      BUILD_ALWAYS 1
      CONFIGURE_COMMAND ""
      BUILD_COMMAND   ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -Dsource_dir:PATH=<SOURCE_DIR> -DMITK_DIR:PATH=${MITK_DIR} -P ${_build_step}
      INSTALL_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -Dsource_dir:PATH=<SOURCE_DIR> -DMITK_DIR:PATH=${MITK_DIR} -P ${_install_step}
      DEPENDS
        ${${proj}_DEPENDENCIES}
    )    
    
    ExternalProject_Add_Step(${proj} forceconfigure
            COMMAND ${CMAKE_COMMAND} -Dsource_dir:PATH=<SOURCE_DIR> -Dpresolver_executable:FILEPATH=${presolver_executable} -Dpython_lib_folder:STRING=${python_lib_folder} -P ${_configure_step}

            DEPENDEES update
            DEPENDERS configure
            ALWAYS 1)

    
    ExternalProject_Get_Property(${proj} source_dir)

       
    set(PythonModules_DIR ${MITK_PYTHON_SITE_DIR}/PythonModules)
    install(SCRIPT ${_install_step})
  else()
    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
  endif()
endif()

