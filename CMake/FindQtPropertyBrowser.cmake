if (NOT QtPropertyBrowser_FOUND AND NOT QtPropertyBrowser_DIR)
    SET(QtPropertyBrowser_DIR "" CACHE PATH "QtPropertyBrowser installation directory" FORCE)
    if (QtPropertyBrowser_FIND_REQUIRED)
        message(FATAL_ERROR "Please set QtPropertyBrowser_DIR variable to the root of the QtPropertyBrowser installation directory")
    endif()
else()
    #find the include dir by looking for Standard_Real.hxx 
    FIND_FILE( QtPropertyBrowser_CONFIG_FILE QtPropertyBrowserConfig.cmake PATHS ${QtPropertyBrowser_DIR} DOC "QtPropertyBrowserConfig.cmake" NO_DEFAULT_PATH) 

    if (NOT QtPropertyBrowser_CONFIG_FILE)
        MESSAGE(FATAL_ERROR "QtPropertyBrowserConfig.cmake not found") 
    endif()
    
    include(${QtPropertyBrowser_CONFIG_FILE})
    include(${QtPropertyBrowser_USE_FILE})

    FIND_LIBRARY(QtPropertyBrowser_LIBRARY_DEBUG QtPropertyBrowser PATHS ${QtPropertyBrowser_LIBRARY_DIRS}/Debug ${QtPropertyBrowser_LIBRARY_DIRS})
    FIND_LIBRARY(QtPropertyBrowser_LIBRARY_RELEASE QtPropertyBrowser PATHS ${QtPropertyBrowser_LIBRARY_DIRS}/Release ${QtPropertyBrowser_LIBRARY_DIRS})

    SET(QtPropertyBrowser_LIBRARIES)
    if (QtPropertyBrowser_LIBRARY_DEBUG) 
        LIST(APPEND QtPropertyBrowser_LIBRARIES debug ${QtPropertyBrowser_LIBRARY_DEBUG}) 
    endif()
    
    if (QtPropertyBrowser_LIBRARY_RELEASE) 
        LIST(APPEND QtPropertyBrowser_LIBRARIES optimized ${QtPropertyBrowser_LIBRARY_RELEASE}) 
    endif()

    set(QtPropertyBrowser_RUNTIME_LIBRARY_DIRS ${QtPropertyBrowser_RUNTIME_LIBRARY_DIRS} CACHE INTERNAL "")
endif()    

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(tPropertyBrowser REQUIRED_VARS QtPropertyBrowser_INCLUDE_DIRS QtPropertyBrowser_LIBRARIES)

MARK_AS_ADVANCED(QtPropertyBrowser_LIBRARIES)
