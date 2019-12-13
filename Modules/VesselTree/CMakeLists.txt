MITK_CREATE_MODULE( VesselTree
  INCLUDE_DIRS Algorithms DataManagement Interactions IO Rendering IO/VesselForestData IO/vtkParametricSplineVesselPath
  DEPENDS MitkSceneSerializationBase CRIMSONUtils
  PACKAGE_DEPENDS PUBLIC Boost GSL
)

IF( BUILD_TESTING )
  add_subdirectory(Testing)
ENDIF()
