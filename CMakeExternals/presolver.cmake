#-----------------------------------------------------------------------------
# OpenCascade
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED presolver_DIR AND NOT EXISTS ${presolver_DIR})
	message(FATAL_ERROR "presolver_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj presolver)
set(proj_DEPENDENCIES )
set(presolver_DEPENDS ${proj})

if(NOT DEFINED presolver_DIR)
	message("presolver_DIR not defined, performing download")
    set(additional_args )
    
    set(presolver_url "")
    if (WIN32)            
        set(presolver_url http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_win.zip)
        set(presolver_download_name presolver_win.zip)
    else ()
        set(presolver_url http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_lin.tar.gz)
        set(presolver_download_name presolver_lin.tar.gz)
    endif()
	
	message("presolver_url is: " ${presolver_url})
	message("downloading presolver source as (presolver_download_name): " ${presolver_download_name})
	
    ExternalProject_Add(${proj} # where proj is presolver
      LIST_SEPARATOR ${sep}
      URL ${presolver_url}
      DOWNLOAD_NAME ${presolver_download_name}
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    find_program(presolver_executable presolver PATHS ${source_dir} NO_DEFAULT_PATH)
    message("Presolver executable path is: " ${presolver_executable})

else()

endif()
