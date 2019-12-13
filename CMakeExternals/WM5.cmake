#-----------------------------------------------------------------------------
# OpenCascade
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED WM5_DIR AND NOT EXISTS ${WM5_DIR})
message(FATAL_ERROR "WM5_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj WM5)
set(proj_DEPENDENCIES )
set(WM5_DEPENDS ${proj})

set(_WM5_BUILD_COMMAND "")
if (WIN32)
    if (MSVC_VERSION EQUAL 1700)
        set(_WM5_VS_VERSION_POSTFIX "110")
    elseif (MSVC_VERSION EQUAL 1800)
        set(_WM5_VS_VERSION_POSTFIX "120")
    else()
        message(ERROR "Unsupported version of MSVS detected: ${MSVC_VERSION}")
    endif()

    # Test 32/64 bits
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(_WM5_VS_BUILD_PLATFORM "x64")
    else()
        set(_WM5_VS_BUILD_PLATFORM "Win32")
    endif()
    
    set (WM5_BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}" 
        "<SOURCE_DIR>/WildMagic5Wgl_VC${_WM5_VS_VERSION_POSTFIX}.sln" 
        "/t:Libraries\\LibCore_VC${_WM5_VS_VERSION_POSTFIX}" 
        "/t:Libraries\\LibMathematics_VC${_WM5_VS_VERSION_POSTFIX}" 
        "/p:Configuration=${CMAKE_CFG_INTDIR}" "/p:Platform=${_WM5_VS_BUILD_PLATFORM}")
        
else()
    set(WM5_BUILD_COMMAND "${CMAKE_MAKE_PROGRAM}" "CFG=${CMAKE_BUILD_TYPE}Dynamic" "--file=makefile.wm5" "--directory=<SOURCE_DIR>/LibCore" 
                  COMMAND "${CMAKE_MAKE_PROGRAM}" "CFG=${CMAKE_BUILD_TYPE}Dynamic" "--file=makefile.wm5" "--directory=<SOURCE_DIR>/LibMathematics")
    #set(WM5_BUILD_COMMAND "pwd")
endif()

if(NOT DEFINED WM5_DIR)
    set(additional_args )

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      URL http://www.isd.kcl.ac.uk/cafa/CRIMSON-superbuild/WildMagic5p13.zip
      DOWNLOAD_NAME WildMagic5p13.zip 
      #URL_MD5 ba87fe9f5ca47e3dfd62aad7223f0e7f
      
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ${WM5_BUILD_COMMAND} 
      INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/SDK <INSTALL_DIR>/SDK
      
      DEPENDS ${proj_DEPENDENCIES})

    ExternalProject_Get_Property(${proj} install_dir)
    set(WM5_DIR "${install_dir}/SDK")
    #set(WM5_DIR ${ep_prefix})
    #mitkFunctionInstallExternalCMakeProject(${proj})

else()
    #mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()
