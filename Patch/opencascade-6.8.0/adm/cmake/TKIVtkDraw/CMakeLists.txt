project(TKIVtkDraw)

if (WIN32)
  list( APPEND TKIVtkDraw_PRECOMPILED_DEFS "-D__IVtkDraw_DLL" )
  string( REGEX REPLACE ";" " " TKIVtkDraw_PRECOMPILED_DEFS "${TKIVtkDraw_PRECOMPILED_DEFS}")
endif()

  list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${WOK_LIB_PATH}" )
  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "" AND EXISTS "${BUILD_PATCH_DIR}/inc")
    list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${BUILD_PATCH_DIR}/inc" )
  ENDIF()
  IF(EXISTS "${CMAKE_SOURCE_DIR}/inc")
    list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${CMAKE_SOURCE_DIR}/inc" )
  ENDIF()

  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "")
    IF(EXISTS "${BUILD_PATCH_DIR}/drv/IVtkDraw")
      list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${BUILD_PATCH_DIR}/drv/IVtkDraw" )
    ENDIF()
    IF(EXISTS "${BUILD_PATCH_DIR}/src/IVtkDraw")
      list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${BUILD_PATCH_DIR}/src/IVtkDraw" )
    ENDIF()

  ENDIF()

  IF(EXISTS "${CMAKE_SOURCE_DIR}/drv/IVtkDraw")
    list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${CMAKE_SOURCE_DIR}/drv/IVtkDraw" )
  ENDIF()
  IF(EXISTS "${CMAKE_SOURCE_DIR}/src/IVtkDraw")
    list( APPEND TKIVtkDraw_COMPILER_DIRECTORIES "${CMAKE_SOURCE_DIR}/src/IVtkDraw" )
  ENDIF()

IF (WIN32)
  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "")
  ENDIF()

ELSE()
  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "")
  ENDIF()

ENDIF()

  list( APPEND TKIVtkDraw_USED_LIBS TKernel )
  list( APPEND TKIVtkDraw_USED_LIBS TKBRep )
  list( APPEND TKIVtkDraw_USED_LIBS TKDraw )
  list( APPEND TKIVtkDraw_USED_LIBS TKG2d )
  list( APPEND TKIVtkDraw_USED_LIBS TKG3d )
  list( APPEND TKIVtkDraw_USED_LIBS TKGeomAlgo )
  list( APPEND TKIVtkDraw_USED_LIBS TKGeomBase )
  list( APPEND TKIVtkDraw_USED_LIBS TKMath )
  list( APPEND TKIVtkDraw_USED_LIBS TKMesh )
  list( APPEND TKIVtkDraw_USED_LIBS TKService )
  list( APPEND TKIVtkDraw_USED_LIBS TKTopAlgo )
  list( APPEND TKIVtkDraw_USED_LIBS TKV3d )
  list( APPEND TKIVtkDraw_USED_LIBS TKOpenGl )
  list( APPEND TKIVtkDraw_USED_LIBS TKIVtk )

  list( APPEND TKIVtkDraw_USED_LIBS ${VTK_LIBRARIES} )

if (WIN32)
  list( APPEND TKIVtkDraw_USED_LIBS tcl${3RDPARTY_TCL_LIBRARY_VERSION}.lib )
  list( APPEND TKIVtkDraw_USED_LIBS tk${3RDPARTY_TCL_LIBRARY_VERSION}.lib )
elseif(APPLE)
  find_library(FRAMEWORKS_TCL NAMES Tcl)
  list( APPEND TKIVtkDraw_USED_LIBS ${FRAMEWORKS_TCL} )
  find_library(FRAMEWORKS_TK NAMES Tk)
  list( APPEND TKIVtkDraw_USED_LIBS ${FRAMEWORKS_TK} )
elseif(ANDROID)
  list( APPEND TKIVtkDraw_USED_LIBS tcl8.6 )
  list( APPEND TKIVtkDraw_USED_LIBS X11 )
  list( APPEND TKIVtkDraw_USED_LIBS tk8.6 )
else()
  list( APPEND TKIVtkDraw_USED_LIBS tcl${3RDPARTY_TCL_LIBRARY_VERSION} )
  list( APPEND TKIVtkDraw_USED_LIBS X11 )
  list( APPEND TKIVtkDraw_USED_LIBS tk${3RDPARTY_TK_LIBRARY_VERSION} )
endif()

  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "" AND EXISTS "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw.cxx")
    list( APPEND TKIVtkDraw_USED_SRCFILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw.cxx" )
    SOURCE_GROUP (IVtkDraw FILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw.cxx")
  ELSE()
    IF(EXISTS "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw.cxx")
      list( APPEND TKIVtkDraw_USED_SRCFILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw.cxx" )
      SOURCE_GROUP (IVtkDraw FILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw.cxx")
    ENDIF()
  ENDIF()

  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "" AND EXISTS "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx")
    list( APPEND TKIVtkDraw_USED_SRCFILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx" )
    SOURCE_GROUP (IVtkDraw FILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx")
  ELSE()
    IF(EXISTS "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx")
      list( APPEND TKIVtkDraw_USED_SRCFILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx" )
      SOURCE_GROUP (IVtkDraw FILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_HighlightAndSelectionPipeline.cxx")
    ENDIF()
  ENDIF()

  IF(NOT "${BUILD_PATCH_DIR}" STREQUAL "" AND EXISTS "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx")
    list( APPEND TKIVtkDraw_USED_SRCFILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx" )
    SOURCE_GROUP (IVtkDraw FILES "${BUILD_PATCH_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx")
  ELSE()
    IF(EXISTS "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx")
      list( APPEND TKIVtkDraw_USED_SRCFILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx" )
      SOURCE_GROUP (IVtkDraw FILES "${CMAKE_SOURCE_DIR}//src/IVtkDraw/IVtkDraw_Interactor.cxx")
    ENDIF()
  ENDIF()


IF (WIN32)
ELSE()

  IF (APPLE)

  ENDIF()

ENDIF()

if ("${USED_TOOLKITS}" STREQUAL "" OR DEFINED TurnONtheTKIVtkDraw)
 add_library( TKIVtkDraw SHARED ${TKIVtkDraw_USED_SRCFILES} )

 set_property(TARGET TKIVtkDraw PROPERTY FOLDER Draw)

 install( TARGETS TKIVtkDraw
                                 RUNTIME DESTINATION "${INSTALL_DIR}/${OS_WITH_BIT}/${COMPILER}/bin${BUILD_POSTFIX}"
                                 ARCHIVE DESTINATION "${INSTALL_DIR}/${OS_WITH_BIT}/${COMPILER}/lib${BUILD_POSTFIX}"
                                 LIBRARY DESTINATION "${INSTALL_DIR}/${OS_WITH_BIT}/${COMPILER}/lib${BUILD_POSTFIX}")

 if (MSVC)
  install( FILES  ${CMAKE_BINARY_DIR}/out/bind/Debug/TKIVtkDraw.pdb CONFIGURATIONS Debug
                                  DESTINATION "${INSTALL_DIR}/${OS_WITH_BIT}/${COMPILER}/bin${BUILD_POSTFIX}")
 endif()


 set_target_properties( TKIVtkDraw PROPERTIES COMPILE_FLAGS "${TKIVtkDraw_PRECOMPILED_DEFS}" )
 include_directories( ${TKIVtkDraw_COMPILER_DIRECTORIES} )
 target_link_libraries( TKIVtkDraw ${TKIVtkDraw_USED_LIBS} )
endif()
