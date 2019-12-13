MITK_CREATE_MODULE( PythonSolverSetupService
  DEPENDS MitkPython MitkPythonService MitkQtPython SolverSetupService ${CRIMSON_MESHING_KERNEL}MeshingKernel PCMRIKernel
  PACKAGE_DEPENDS PRIVATE Qt5|Widgets+OpenGL QtPropertyBrowser PUBLIC GSL Eigen
)

#IF( BUILD_TESTING )
#  add_subdirectory(Testing)
#ENDIF()
