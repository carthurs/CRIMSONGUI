MITK_CREATE_MODULE( PCMRIKernel
  INCLUDE_DIRS DataManagement ExtensionPoint ExtensionPoint/internal Rendering IO # Initialization Interactions
  DEPENDS MitkSceneSerializationBase MitkPlanarFigure MitkSegmentation AsyncTask MitkContourModel SolidKernel ${CRIMSON_MESHING_KERNEL}MeshingKernel
  PACKAGE_DEPENDS PUBLIC GSL Boost 
  PRIVATE WM5 Eigen3 Eigen3|SPQR Qt5 Qt5|Core #Cholmod
)

IF( BUILD_TESTING )
#  add_subdirectory(Testing)
ENDIF()