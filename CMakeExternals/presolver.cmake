#-----------------------------------------------------------------------------
# Presolver (download only)
#-----------------------------------------------------------------------------

message("Start of CMakeExternals/Presolver.cmake")

# Sanity checks
if(DEFINED presolver_DIR AND NOT EXISTS ${presolver_DIR})
message(FATAL_ERROR "presolver_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj presolver)
set(proj_DEPENDENCIES )
set(presolver_DEPENDS ${proj})

if(NOT DEFINED presolver_DIR)
    set(additional_args )
    
    # In the Superbuild the presolver is just downloaded
    if (WIN32)
        # NOTE: For the scalar UI this version is much too old, do not use this.
        # old URL: http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_win.zip)
        set(presolver_download_name presolver_win.zip)
    else ()
        # NOTE: For the scalar UI this version is much too old, do not use this.
        # old URL:  http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/presolver_lin.tar.gz)
        set(presolver_download_name presolver_lin.tar.gz)
    endif()

    set(presolver_url "replace/me/with/path/to/presolver_win.zip-NOTFOUND" CACHE FILEPATH  
        "Path (or URL) where the (MinGW built) presolver_win.zip or presolver_lin.tar.gz can be found. This zip file should contain the presolver executable.")

    message("presolver_url is " ${presolver_url})
    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL ${presolver_url}
      DOWNLOAD_NAME ${presolver_download_name}
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    find_program(presolver_executable presolver PATHS ${source_dir} NO_DEFAULT_PATH)
endif()

message("End of CMakeExternals/Presolver.cmake")