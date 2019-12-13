#-----------------------------------------------------------------------------
# Guideline support library
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED GSL_INCLUDE_DIR AND NOT EXISTS ${GSL_INCLUDE_DIR})
message(FATAL_ERROR "GSL_INCLUDE_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj GSL)
set(proj_DEPENDENCIES )
set(GSL_DEPENDS ${proj})

if(NOT DEFINED GSL_INCLUDE_DIR)
    set(additional_args )
    
    if (MSVC)
        set(_gsl_url https://github.com/rkhlebnikov/GSL.git)
    else()
        set(_gsl_url https://github.com/rkhlebnikov/gsl-lite.git)
    endif()

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      GIT_REPOSITORY ${_gsl_url}

      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    set(GSL_INCLUDE_DIR ${source_dir}/include)
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()