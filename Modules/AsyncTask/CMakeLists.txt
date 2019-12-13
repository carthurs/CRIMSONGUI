MITK_CREATE_MODULE( AsyncTask 
    PACKAGE_DEPENDS PUBLIC GSL Boost )

IF( BUILD_TESTING )
#add_subdirectory(Testing)
ENDIF()
