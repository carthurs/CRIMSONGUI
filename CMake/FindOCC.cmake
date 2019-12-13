# - Try to find OCC libraries 
### Does not test what version has been found,though 
### that could be done by parsing Standard_Version.hxx 
 
 
# Once done, this will define 
#  OCC_FOUND - true if OCC has been found 
#  OCC_INCLUDE_DIR - the OCC include dir 
#  OCC_LIBRARIES - names of OCC libraries 
#  OCC_LINK_DIRECTORY - location of OCC libraries 

if (NOT OCC_FOUND AND NOT OCC_DIR)
    SET(OCC_DIR "" CACHE PATH "OpenCascade installation directory" FORCE)
    if (OCC_FIND_REQUIRED)
        message(FATAL_ERROR "Please set OCC_DIR variable to the root of the OpenCascade installation directory")
    endif()
else()
    ########################################################
    ### BEGIN FROM OpenCASCADE's CMakeLists.txt

    if (MSVC)
      if (MSVC70)
        set (COMPILER vc7)
      elseif (MSVC80)
        set (COMPILER vc8)
      elseif (MSVC90)
        set (COMPILER vc9)
      elseif (MSVC10)
        set (COMPILER vc10)
      elseif (MSVC11)
        set (COMPILER vc11)
      elseif (MSVC12)
        set (COMPILER vc12)
      endif()
    elseif (DEFINED CMAKE_COMPILER_IS_GNUCC)
      set (COMPILER gcc)
    elseif (DEFINED CMAKE_COMPILER_IS_GNUCXX)
      set (COMPILER gxx)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set (COMPILER clang)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
      set (COMPILER icc)
    else()
      set (COMPILER ${CMAKE_GENERATOR})
      string (REGEX REPLACE " " "" COMPILER ${COMPILER})
    endif()


    MATH(EXPR COMPILER_BITNESS "32 + 32*(${CMAKE_SIZEOF_VOID_P}/8)")
    if (WIN32)
      SET(OS_WITH_BIT "win${COMPILER_BITNESS}")
    elseif(APPLE)
      SET(OS_WITH_BIT "mac${COMPILER_BITNESS}")
    else()
      SET(OS_WITH_BIT "lin${COMPILER_BITNESS}")
    endif()
    
    ### END FROM OpenCASCADE's CMakeLists.txt
    ########################################################

    SET(BUILD_SUFFIX_debug "d")
    SET(BUILD_SUFFIX_release "")

    #find the include dir by looking for Standard_Real.hxx 
    FIND_PATH( OCC_INCLUDE_DIR Standard_Real.hxx PATHS ${OCC_DIR}/inc DOC "Path to OCC includes" NO_DEFAULT_PATH) 

    FIND_PATH( OCC_LINK_DIRECTORY_debug NAMES TKernel.lib libTKernel.so PATHS "${OCC_DIR}/${OS_WITH_BIT}/${COMPILER}/lib${BUILD_SUFFIX_debug}" DOC "Path to OCC debug libs" NO_DEFAULT_PATH) 
    FIND_PATH( OCC_LINK_DIRECTORY_release NAMES TKernel.lib libTKernel.so PATHS "${OCC_DIR}/${OS_WITH_BIT}/${COMPILER}/lib${BUILD_SUFFIX_release}"  DOC "Path to OCC release libs" NO_DEFAULT_PATH) 
    FIND_PATH( OCC_BINARY_DIRECTORY_debug NAMES TKernel.dll libTKernel.so PATHS "${OCC_DIR}/${OS_WITH_BIT}/${COMPILER}/bin${BUILD_SUFFIX_debug}" DOC "Path to OCC debug shared object files" NO_DEFAULT_PATH) 
    FIND_PATH( OCC_BINARY_DIRECTORY_release NAMES TKernel.dll libTKernel.so PATHS "${OCC_DIR}/${OS_WITH_BIT}/${COMPILER}/bin${BUILD_SUFFIX_release}" DOC "Path to OCC release shared object files" NO_DEFAULT_PATH) 
    MARK_AS_ADVANCED(OCC_INCLUDE_DIR)
    MARK_AS_ADVANCED(OCC_LINK_DIRECTORY_debug)
    MARK_AS_ADVANCED(OCC_LINK_DIRECTORY_release)
    MARK_AS_ADVANCED(OCC_BINARY_DIRECTORY_debug)
    MARK_AS_ADVANCED(OCC_BINARY_DIRECTORY_release)

    IF ( OCC_INCLUDE_DIR AND (OCC_LINK_DIRECTORY_debug OR OCC_LINK_DIRECTORY_release) ) 

        IF( NOT OCC_FIND_COMPONENTS ) 
            set(OCC_FIND_COMPONENTS TKFillet TKMesh TKernel TKG2d TKG3d TKMath TKIGES TKSTL TKShHealing TKXSBase TKBool TKBO TKBRep TKTopAlgo TKGeomAlgo TKGeomBase TKOffset TKPrim TKSTEP TKSTEPBase TKSTEPAttr TKHLR TKFeat TKNIS TKCAF TKLCAF)
        ENDIF()                                                                                                                                                                                                                     
        
        FOREACH( _libname ${OCC_FIND_COMPONENTS} ) 
            foreach (conf debug release)
                IF (NOT OCC_${_libname}_${conf}_FOUND)
                    FIND_LIBRARY( ${_libname}_${conf}_OCCLIB ${_libname} PATHS ${OCC_LINK_DIRECTORY_${conf}} NO_DEFAULT_PATH)
                    SET( _foundlib_${conf} ${${_libname}_${conf}_OCCLIB} ) 
                    if (_foundlib_${conf})
                        set(OCC_${_libname}_${conf}_FOUND TRUE)
                        set(OCC_${_libname}_FOUND TRUE)
                    endif()
                    MARK_AS_ADVANCED(${_libname}_${conf}_OCCLIB)
                ENDIF()
            endforeach ()
            if (OCC_${_libname}_debug_FOUND AND OCC_${_libname}_release_FOUND)
                SET(OCC_LIBRARIES ${OCC_LIBRARIES} debug ${${_libname}_debug_OCCLIB} optimized ${${_libname}_release_OCCLIB}) 
            elseif (OCC_${_libname}_debug_FOUND)
                SET(OCC_LIBRARIES ${OCC_LIBRARIES} ${${_libname}_debug_OCCLIB}) 
            elseif (OCC_${_libname}_release_FOUND)
                SET(OCC_LIBRARIES ${OCC_LIBRARIES} ${${_libname}_release_OCCLIB}) 
            endif()
        ENDFOREACH( _libname ${OCC_FIND_COMPONENTS} ) 

        IF (UNIX) 
            ADD_DEFINITIONS( -DLIN -DLININTEL ) 
        ELSEIF (WIN32) 
            ADD_DEFINITIONS( -DWNT ) 
        ENDIF (UNIX) 

        # 32 bit or 64 bit? 
        IF( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
            ADD_DEFINITIONS( -D_OCC64 ) 
            IF (UNIX) 
                ADD_DEFINITIONS( -m64 ) 
            ENDIF (UNIX) 
        ENDIF( ) 

        ADD_DEFINITIONS( -DHAVE_CONFIG_H -DHAVE_IOSTREAM -DHAVE_FSTREAM -DHAVE_LIMITS_H ) 
    ENDIF( ) 
endif()    

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OCC REQUIRED_VARS OCC_INCLUDE_DIR OCC_LIBRARIES HANDLE_COMPONENTS)

MARK_AS_ADVANCED(OCC_LIBRARIES)
