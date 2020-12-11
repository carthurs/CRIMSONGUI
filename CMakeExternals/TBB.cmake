#-----------------------------------------------------------------------------
# OpenCascade
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED TBB_DIR AND NOT EXISTS ${TBB_DIR})
message(FATAL_ERROR "TBB_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj TBB)
set(proj_DEPENDENCIES )
set(TBB_DEPENDS ${proj})

if(NOT DEFINED TBB_DIR)
    set(additional_args )

    if (WIN32)
        set(TBB_URL  https://github.com/01org/tbb/releases/download/2018_U2/tbb2018_20171205oss_win.zip)
    else()
        set(TBB_URL "https://www.threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb43_20150611oss_lin.tgz")
    endif()
    
    ExternalProject_Add(${proj} # where proj is TBB
      LIST_SEPARATOR ${sep}
      URL ${TBB_URL}
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} source_dir)
    set(TBB_DIR ${source_dir})

else()

endif()
