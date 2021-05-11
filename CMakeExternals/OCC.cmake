#-----------------------------------------------------------------------------
# OpenCascade
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED OCC_DIR AND NOT EXISTS ${OCC_DIR})
message(FATAL_ERROR "OCC_DIR variable is defined but corresponds to non-existing directory")
endif()

macro(ONE_CONFIGURATION_BUILD_COMMAND out_var Config Step)
set(${out_var}
    ${CMAKE_COMMAND}
      -DDESIRED_CONFIGURATION=${Config}
      -DSTEP=${Step}
      -DBINARY_DIR=<BINARY_DIR>
      -DCMAKE_CFG_INTDIR=${CMAKE_CFG_INTDIR}
      -P ${CMAKE_CURRENT_LIST_DIR}/BuildOneConfiguration.cmake)
endmacro()

macro(Add_OneConfiguration_ExternalProject Config)
    ONE_CONFIGURATION_BUILD_COMMAND(_build_command ${Config} BUILD)
    ONE_CONFIGURATION_BUILD_COMMAND(_install_command ${Config} INSTALL)
    
    if (${Config} STREQUAL "Debug")
        set(OtherConfig "Release")
    else()
        set(OtherConfig "Debug")
    endif()

    ExternalProject_Add(
        OCC_${Config}
        LIST_SEPARATOR ${sep}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${source_dir}
        BUILD_COMMAND ${_build_command}
        INSTALL_COMMAND ${_install_command}

        CMAKE_GENERATOR ${gen}
        CMAKE_ARGS
            ${ep_common_args}
            ${additional_args}
            "-DINSTALL_DIR:PATH=${install_dir}"
            "-DBUILD_CONFIGURATION:STRING=${Config}"
            "-DCMAKE_PREFIX_PATH:STRING=${freetype_DIR}/${Config}^^${freetype_DIR}/${OtherConfig}"
        CMAKE_CACHE_ARGS
            ${ep_common_cache_args}
            "-D3RDPARTY_FREETYPE_DLL_DIR:PATH="
            "-D3RDPARTY_FREETYPE_LIBRARY_DIR:PATH="
            "-D3RDPARTY_FREETYPE_DIR:FILEPATH="
            "-D3RDPARTY_FREETYPE_DLL:FILEPATH="
            "-D3RDPARTY_FREETYPE_INCLUDE_DIR_freetype2:PATH="
            "-D3RDPARTY_FREETYPE_INCLUDE_DIR_ft2build:PATH="
            "-D3RDPARTY_FREETYPE_LIBRARY:PATH="
        CMAKE_CACHE_DEFAULT_ARGS
            ${ep_common_cache_default_args}

        DEPENDS OCC_src
          )
endmacro()

set(proj OCC)
set(proj_DEPENDENCIES freetype TBB)
set(OCC_DEPENDS ${proj})

if(NOT DEFINED OCC_DIR)
    set(additional_args             
            "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${OCC_CXX_FLAGS}"
            "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} ${OCC_C_FLAGS}"

            "-DBUILD_ApplicationFramework:BOOL=OFF"
            "-DBUILD_Draw:BOOL=OFF"
            "-DINSTALL_FREETYPE:BOOL=ON"
            
            "-DUSE_TBB:BOOL=ON"
            "-DINSTALL_TBB:BOOL=ON"
            "-D3RDPARTY_TBB_DIR:PATH=${TBB_DIR}"
            "-D3RDPARTY_TBB_INCLUDE_DIR:PATH=${TBB_DIR}/include"
            )


    set(OCC_URL https://github.com/CRIMSONCardiovascularModelling/OCC/archive/refs/tags/6.9.1.tar.gz)
    set(OCC_PATCH_COMMAND  ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/OCC_6.9.1.patch)
    
    if (WIN32)
        set(OCC_CXX_FLAGS "${OCC_CXX_FLAGS} -DWNT")
        
                
        ExternalProject_Add(OCC_src
            LIST_SEPARATOR ${sep}
            URL ${OCC_URL} 
            DOWNLOAD_NAME OCC-6.9.1.tar.gz
            #URL_MD5 ba87fe9f5ca47e3dfd62aad7223f0e7f
            PATCH_COMMAND ${OCC_PATCH_COMMAND}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            DEPENDS ${proj_DEPENDENCIES}
        )

        ExternalProject_Get_Property(OCC_src source_dir)
        ExternalProject_Get_Property(OCC_src binary_dir)
        ExternalProject_Get_Property(OCC_src install_dir)
        
        Add_OneConfiguration_ExternalProject(Debug)
        Add_OneConfiguration_ExternalProject(Release)

        ExternalProject_Add(${proj}
            DOWNLOAD_COMMAND ""
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            DEPENDS OCC_Debug OCC_Release
        )
    else()
        set(_tbb_postfix)
        if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            set(_tbb_postfix "_debug")
        endif()
        ExternalProject_Add(
            ${proj}
            URL ${OCC_URL}
            DOWNLOAD_NAME opencascade-6.9.1.tgz
            PATCH_COMMAND ${OCC_PATCH_COMMAND}

            CMAKE_GENERATOR ${gen}
            CMAKE_ARGS
                ${ep_common_args}
                ${additional_args}
                "-DINSTALL_DIR:PATH=<INSTALL_DIR>"
                "-DBUILD_CONFIGURATION:STRING=${CMAKE_BUILD_TYPE}"
                "-DCMAKE_PREFIX_PATH:PATH=${freetype_DIR}"
                "-D3RDPARTY_TBB_LIBRARY_DIR:PATH=${TBB_DIR}/lib/intel64/gcc4.4"
                "-D3RDPARTY_TBB_LIBRARY:FILEPATH=${TBB_DIR}/lib/intel64/gcc4.4/libtbb${_tbb_postfix}.so.2"
                "-D3RDPARTY_TBBMALLOC_LIBRARY_DIR:PATH=${TBB_DIR}/lib/intel64/gcc4.4"
                "-D3RDPARTY_TBBMALLOC_LIBRARY:FILEPATH=${TBB_DIR}/lib/intel64/gcc4.4/libtbbmalloc${_tbb_postfix}.so.2"


            CMAKE_CACHE_ARGS
                ${ep_common_cache_args}
            CMAKE_CACHE_DEFAULT_ARGS
                ${ep_common_cache_default_args}
            DEPENDS ${proj_DEPENDENCIES}
        )
        ExternalProject_Get_Property(OCC install_dir)
    endif()

    set(OCC_DIR "${install_dir}")
    
    #set(OCC_DIR ${ep_prefix})
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
