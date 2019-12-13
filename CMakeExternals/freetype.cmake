#-----------------------------------------------------------------------------
# OpenCascade
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED freetype_DIR AND NOT EXISTS ${freetype_DIR})
message(FATAL_ERROR "freetype_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj freetype)
set(proj_DEPENDENCIES )
set(freetype_DEPENDS ${proj})

if(NOT DEFINED freetype_DIR)
    set(additional_args )

    set(freetype_patch_step "")
    if (WIN32)
        set(freetype_patch_step   ${PATCH_COMMAND}  -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/freetype_win_dll.patch)
    endif()
    
    set(freetype_install_dir ${ep_install_dir}/${proj})
    
    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL http://download.savannah.gnu.org/releases/freetype/freetype-2.5.5.tar.gz
      # URL_HASH
      PATCH_COMMAND ${freetype_patch_step}
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
         ${ep_common_args}
         ${additional_args}
         "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${freetype_CXX_FLAGS}"
         "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} ${freetype_C_FLAGS}"
         "-DCMAKE_INSTALL_PREFIX:PATH=${freetype_install_dir}/${CMAKE_CFG_INTDIR}"
      CMAKE_CACHE_ARGS
        ${ep_common_cache_args}
      INSTALL_DIR ${freetype_install_dir}
      CMAKE_CACHE_DEFAULT_ARGS
        ${ep_common_cache_default_args}
      DEPENDS ${proj_DEPENDENCIES}
      )

    set(freetype_DIR ${freetype_install_dir})
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
