#-----------------------------------------------------------------------------
# Flowsolver
#-----------------------------------------------------------------------------

if(NOT PACKAGE_FLOWSOLVER)
  return()
endif()


# Sanity checks
if(DEFINED flowsolver_DIR AND NOT EXISTS ${flowsolver_DIR})
message(FATAL_ERROR "flowsolver_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj flowsolver)
set(proj_DEPENDENCIES )
set(flowsolver_DEPENDS ${proj})

if(NOT DEFINED flowsolver_DIR)
    set(additional_args )
    

	ExternalProject_Add(${proj}
	  LIST_SEPARATOR ${sep}
	  DOWNLOAD_COMMAND ""
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    find_path(flowsolver_folder flowsolver.exe PATHS ${source_dir} NO_DEFAULT_PATH)
    
    #set(presolver_DIR ${ep_prefix})
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
