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
    set(additional_args )
    
    set(presolver_url "")
    if (WIN32)            
        set(presolver_url http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_win.zip)
        set(presolver_download_name presolver_win.zip)
    else ()
        set(presolver_url http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_lin.tar.gz)
        set(presolver_download_name presolver_lin.tar.gz)
    endif()

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL ${presolver_url}
      DOWNLOAD_NAME ${presolver_download_name}
      #URL_MD5 ba87fe9f5ca47e3dfd62aad7223f0e7f
      #PATCH_COMMAND patch -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/presolver_6.9.0.patch
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    find_program(presolver_executable presolver PATHS ${source_dir} NO_DEFAULT_PATH)
    
    #set(presolver_DIR ${ep_prefix})
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
