# - Try to find WildMagic5 Lbrary library installation
#
# The follwoing variables are optionally searched for defaults
#  WM5_DIR:            Base directory of WM5 tree to use (the installation directory after the compilation of Wm5 INSTALL project).
#
# The following are set after configuration is done: 
#  WM5_FOUND
#  WM5_INCLUDE_DIR
#  WM5_LIBRARIES
#  WM5_LINK_DIRECTORIES
#
# 
# 2011/07 Pierluigi Taddei  pierlugi.taddei@polimi.it

MACRO(DBG_MSG _MSG)
   #MESSAGE(STATUS "${CMAKE_CURRENT_LIST_FILE}:\n${_MSG}")
ENDMACRO(DBG_MSG)


# required  components with header and library if COMPONENTS unspecified
IF    (NOT WM5_FIND_COMPONENTS)
  #default
  SET(WM5_FIND_COMPONENTS Wm5Core Wm5Mathematics Wm5Physics Wm5Imagics Wm5WglGraphics  )
ENDIF (NOT WM5_FIND_COMPONENTS)


SET(WM5_COMPONENTS Wm5Core Wm5Mathematics Wm5Physics Wm5Imagics Wm5WglGraphics )


# typical root dirs of installations, exactly one of them is used
SET (WM5_POSSIBLE_ROOT_DIRS
	$ENV{WM5_DIR} $ENV{WM5_PATH}
	/usr/local
	/usr
  )


#
# select exactly ONE WM5 base directory/tree 
# to avoid mixing different version headers and libs
#
FIND_PATH(WM5_ROOT_DIR   NAMES   Include/Wm5Core.h  PATHS ${WM5_POSSIBLE_ROOT_DIRS})

SET(WM5_INCLUDE_DIR "${WM5_ROOT_DIR}/Include")

DBG_MSG("WM5_ROOT_DIR=${WM5_ROOT_DIR}")
DBG_MSG("WM5_INCLUDE_DIR=${WM5_INCLUDE_DIR}")

#
# find sbsolute path to all libraries 
# some are optionally, some may not exist on Linux
#

## Loop over each components
SET(WM5_FOUND_TMP true)

FOREACH(_lib ${WM5_FIND_COMPONENTS})
	#is this component required?
	STRING(TOUPPER WM5_${_lib} _WM5LIB)
	LIST(FIND WM5_FIND_COMPONENTS ${_lib} _PRESENT)
	DBG_MSG( "${_WM5LIB} ?  ${_PRESENT}")
	IF(NOT(${_PRESENT} EQUAL -1 ))
	
	    set(_WM5_DLL_POSTFIX)
	
	    if (MSVC_VERSION)
    	    set(_WM5_DLL_POSTFIX "") # Use static libraries in Windows

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
            
            set(WM5_VS_SEARCH_PATH ${WM5_ROOT_DIR}/Library/v${_WM5_VS_VERSION_POSTFIX}/${_WM5_VS_BUILD_PLATFORM})
        else()
    	    set(_WM5_DLL_POSTFIX "Dynamic")
        endif()
		
		#find the debug library
		FIND_LIBRARY(${_WM5LIB}_DEBUG_LIBRARY 
		    NAMES "${_lib}${WM5_PLATFORM_SUFFIX}d" "${_lib}${WM5_PLATFORM_SUFFIX}" # In Linux, debug build has no "d" postfix
		    PATHS ${WM5_VS_SEARCH_PATH}/Debug${_WM5_DLL_POSTFIX} ${WM5_ROOT_DIR}/Library/Debug${_WM5_DLL_POSTFIX} PATH_SUFFIXES lib  NO_CMAKE_SYSTEM_PATH )
		    
		#find the release library
		FIND_LIBRARY(${_WM5LIB}_RELEASE_LIBRARY 
		    NAMES "${_lib}${WM5_PLATFORM_SUFFIX}"
		    PATHS ${WM5_VS_SEARCH_PATH}/Release${_WM5_DLL_POSTFIX} ${WM5_ROOT_DIR}/Library/Release${_WM5_DLL_POSTFIX} PATH_SUFFIXES lib  NO_CMAKE_SYSTEM_PATH )
		
		#Remove the cache value
		SET(${_WM5LIB}_LIBRARY "" CACHE STRING "" FORCE)
			
		#both debug/release
		if(${_WM5LIB}_DEBUG_LIBRARY AND ${_WM5LIB}_RELEASE_LIBRARY)
			SET(${_WM5LIB}_LIBRARY debug ${${_WM5LIB}_DEBUG_LIBRARY} optimized ${${_WM5LIB}_RELEASE_LIBRARY}  CACHE STRING "" FORCE)
		#only debug
		elseif(${_WM5LIB}_DEBUG_LIBRARY)
			SET(${_WM5LIB}_LIBRARY ${${_WM5LIB}_DEBUG_LIBRARY}  CACHE STRING "" FORCE)
		#only release
		elseif(${_WM5LIB}_RELEASE_LIBRARY)
			SET(${_WM5LIB}_LIBRARY ${${_WM5LIB}_RELEASE_LIBRARY}  CACHE STRING "" FORCE)
		#no library found
		else()
			message(STATUS "WARNING: ${_lib}${WM5_PLATFORM_SUFFIX} was not found.")
			SET(WM5_FOUND_TMP false)
		endif()
			
		#Add to the general list
		if(${_WM5LIB}_LIBRARY)
			SET(WM5_LIBRARIES ${WM5_LIBRARIES} ${${_WM5LIB}_LIBRARY})
		endif()
		
	ENDIF(NOT(${_PRESENT} EQUAL -1 ))
ENDFOREACH(_lib ${WM5_COMPONENTS})

SET(WM5_FOUND ${WM5_FOUND_TMP} CACHE BOOL "" FORCE)

DBG_MSG( "${WM5_LIBRARIES}")
##====================================================
## Print message
if(NOT WM5_FOUND)
  # make FIND_PACKAGE friendly
  if(NOT WM5_FIND_QUIETLY)
        if(WM5_FIND_REQUIRED)
          message(FATAL_ERROR "WM5 required but some headers or libs not found. ${ERR_MSG}")
        else(WM5_FIND_REQUIRED)
          message(STATUS "WARNING: WM5 was not found. ${ERR_MSG}")
        endif(WM5_FIND_REQUIRED)
  endif(NOT WM5_FIND_QUIETLY)
endif(NOT WM5_FOUND)
##====================================================

DBG_MSG("WM5_LIBRARIES=${WM5_LIBRARIES}")

# get the link directory for rpath to be used with LINK_DIRECTORIES: 
IF    (WM5_LIBRARY)
  GET_FILENAME_COMPONENT(WM5_LINK_DIRECTORIES ${WM5_LIBRARY} PATH)
ENDIF (WM5_LIBRARY)


# make FIND_PACKAGE case sensitive compatible
SET(WM5_FOUND       ${WM5_FOUND})
SET(WM5_LIBRARIES   ${WM5_LIBRARIES})
SET(WM5_INCLUDE_DIR ${WM5_INCLUDE_DIR})

MARK_AS_ADVANCED(  WM5_ROOT_DIR   WM5_INCLUDE_DIR  WM5_LIBRARIES  )
