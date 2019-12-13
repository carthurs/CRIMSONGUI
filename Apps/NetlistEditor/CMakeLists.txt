project( CRIMSONBCT )
cmake_minimum_required( VERSION 2.6 )

set(
  CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/cmake/Modules/"
)
set( CMAKE_BUILD_TYPE "Release" )

set( PROJECT_VERSION_MAJOR 0 )
set( PROJECT_VERSION_MINOR 1 )
set( PROJECT_VERSION_PATCH 0 )
set( PROJECT_VERSION_REVISION 1 )
set( PROJECT_VERSION
  ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

set( PACKAGE_VERSION "${PROJECT_VERSION}" )
set( PACKAGE_NAME "CRIMSON Boundary Condition Toolbox" )
set( SETTINGS_ORGANIZATION "Unifi" )
set( SETTINGS_APPLICATION "${PACKAGE_NAME}" )
set( PROJECT_AUTHOR "Michele Caini" )
set( PROJECT_YEAR_COPYRIGHT "2009-2011" )
set( PROJECT_AUTHOR_EMAIL "michele.caini@gmail.com" )
set( PACKAGE_CHEF "${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>" )

# Uninstall Target
configure_file("${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# add_definitions( -ansi -Wall -O0 -g )
# add_definitions( -ansi -Wall -O2 )
add_definitions(
  -DPACKAGE_NAME="${PACKAGE_NAME}"
  -DPACKAGE_VERSION="${PACKAGE_VERSION}"
)
if(WIN32)
  add_definitions( -static )
endif(WIN32)

if(UNIX)
  set( UNIX_SYS TRUE )
endif(UNIX)

if(WIN32)
  set( WIN32_SYS TRUE )
endif(WIN32)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in src/config.h )

add_subdirectory( src )

# create the debian.tar.gz file with the debian subfolder
find_program(TAR_EXECUTABLE tar)
if( TAR_EXECUTABLE )
  add_custom_target(debian_tar
    ${TAR_EXECUTABLE} -cz --exclude=.svn -f "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}-debian.tar.gz" "debian/"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Create the tarball with the debian subfolder only.")
else( TAR_EXECUTABLE )
  # TODO this code doesn't exclude the .svn subfolders, try to fix!!
  add_custom_target(debian_tar
    ${CMAKE_COMMAND} -E tar czf "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}-debian.tar.gz" "debian/"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Create the tarball with the debian subfolder only.")
endif( TAR_EXECUTABLE )
mark_as_advanced( TAR_EXECUTABLE )


#################################################################
##################                             ##################
##################            CPACK            ##################
##################                             ##################
#################################################################

if(INCLUDE_QT_DLL_IN_BIN_PACKAGE OR INCLUDE_QWT5_DLL_IN_BIN_PACKAGE OR NOT QT_CONFIG MATCHES "static")
	include( InstallRequiredSystemLibraries )
endif(INCLUDE_QT_DLL_IN_BIN_PACKAGE OR INCLUDE_QWT5_DLL_IN_BIN_PACKAGE OR NOT QT_CONFIG MATCHES "static")


set( CPACK_GENERATOR "TZ;TGZ;TBZ2" )

set( CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}-source" )
set( CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}" )
set( CPACK_PACKAGE_NAME "${PACKAGE_NAME}" )
set( CPACK_PACKAGE_VENDOR "${PACKAGE_CHEF}" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "QSapecNG - Qt-based GUI for SapecNG" )
SET( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )
SET( CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" )
set( CPACK_SOURCE_IGNORE_FILES
  .*~
  .*kdev*
  /CMakeFiles/
  /_CPack_Packages/
  /\\\\.svn/
  /\\\\.DS_Store
  /\\\\.Trashes/
  \\\\.cproject
  \\\\.project
  CMakeCache\\\\.txt
  build.*
  \\\\.settings
  .*\\\\.patch
  moc_qtbuttonpropertybrowser\\\\.cpp
  moc_qteditorfactory\\\\.cpp
  moc_qtgroupboxpropertybrowser\\\\.cpp
  moc_qtpropertybrowser\\\\.cpp
  moc_qtpropertybrowserutils_p\\\\.cpp
  moc_qtpropertymanager\\\\.cpp
  moc_qttreepropertybrowser\\\\.cpp
  moc_qtvariantproperty\\\\.cpp
  qteditorfactory\\\\.moc
  qtpropertymanager\\\\.moc
  qttreepropertybrowser\\\\.moc
  /\\\\.pc/
  "${CMAKE_SOURCE_DIR}/debian/"
)
set(
  CPACK_PACKAGE_INSTALL_DIRECTORY
  "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)
set(
  CPACK_INSTALL_CMAKE_PROJECTS
  ${CMAKE_CURRENT_BINARY_DIR};${PACKAGE_NAME};ALL;/
)


if(WIN32 AND NOT UNIX)

  set( CPACK_PACKAGE_EXECUTABLES "${OUTPUT_BIN_NAME}" "${OUTPUT_BIN_NAME}" )
  set( CPACK_NSIS_INSTALLED_ICON_NAME "${OUTPUT_BIN_FOLDER}\\\\${OUTPUT_BIN_NAME}.exe" )
  set( CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}" )
  set( CPACK_NSIS_CONTACT "${PACKAGE_CHEF}" )
  set( CPACK_NSIS_URL_INFO_ABOUT "http://qsapecng.sourceforge.net/" )
  set( CPACK_NSIS_MODIFY_PATH ON )
  set( CPACK_GENERATOR NSIS ZIP )
  set( CPACK_SOURCE_GENERATOR ZIP )
  set(
    CPACK_NSIS_MUI_ICON
    "${CMAKE_CURRENT_SOURCE_DIR}\\\\src\\\\gui\\\\images\\\\icon.ico"
  )
  set(
    CPACK_NSIS_MUI_UNIICON
    "${CMAKE_CURRENT_SOURCE_DIR}\\\\src\\\\gui\\\\images\\\\icon.ico"
  )
  
  # la variabile CPACK_PACKAGE_ICON vuole una bmp da 150x57 pixel
  # la variabile CPACK_NSIS_INSTALLER_MUI_ICON_CODE vuole una bmp da 164x314 pixel
  #set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\src\\\\gui\\\\images\\\\qsapecng_nsis_header.bmp" )
  #set( CPACK_NSIS_INSTALLER_MUI_ICON_CODE "!define MUI_WELCOMEFINISHPAGE_BITMAP '${CMAKE_CURRENT_SOURCE_DIR}\\\\src\\\\gui\\\\images\\\\qsapecng_nsis_welcome.bmp'" )
  
  # Codice per far creare il link sul desktop se l'utente lo seleziona durante
  # l'installazione da setup: CMake crea un checkbox che non funziona!!!
  set( CPACK_NSIS_DESKTOP_LINK_NAME "QSapecNG" )
  set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "StrCmp $INSTALL_DESKTOP '1' 0 nodesktoplink\nCreateShortcut '$desktop\\\\${CPACK_NSIS_DESKTOP_LINK_NAME}.lnk' '$instdir\\\\${OUTPUT_BIN_FOLDER}\\\\${OUTPUT_BIN_NAME}.exe'\nnodesktoplink:\n" )
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "StrCmp $INSTALL_DESKTOP '1' 0 nodesktoplink\nDelete '$desktop\\\\${CPACK_NSIS_DESKTOP_LINK_NAME}.lnk'\nnodesktoplink:\n" )

elseif(APPLE)

  set( CPACK_DMG_VOLUME_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}" ) # da testare
  set( CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}" )
  set( CPACK_GENERATOR DragNDrop )
  set( CPACK_SOURCE_GENERATOR TGZ ZIP )

else()

  set( CPACK_STRIP_FILES "usr/${OUTPUT_BIN_FOLDER}/${OUTPUT_BIN_NAME}" )
  set( CPACK_SOURCE_STRIP_FILES "" )

  set( CPACK_GENERATOR "${CPACK_GENERATOR};DEB" )
  set( CPACK_SOURCE_GENERATOR TBZ2 TGZ ZIP)
  set( CPACK_DEBIAN_PACKAGE_MAINTAINER "${PACKAGE_CHEF}" )
  set( CPACK_DEBIAN_PACKAGE_SECTION Electronics )
  set( CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}+sid" )
  set(
    CPACK_DEBIAN_PACKAGE_DEPENDS
     "libstdc++6 (>=4.3.2-1.1),
      libqt4-core (>=4.6),
      libqt4-gui (>=4.6),
      libqwt5-qt4 (>=5.0)"
  )

endif(WIN32 AND NOT UNIX)


include( CPack )
