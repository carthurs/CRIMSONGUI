MITK_CREATE_MODULE( SolidKernel
  INCLUDE_DIRS DataManagement ExtensionPoint IO Rendering Interactions
  DEPENDS MitkSceneSerializationBase MitkPlanarFigure VesselTree AsyncTask MitkContourModel MitkQtWidgetsExt
  PACKAGE_DEPENDS PUBLIC GSL OCC Boost 
  PRIVATE WM5
)

IF( BUILD_TESTING )
  add_subdirectory(Testing)
ENDIF()
