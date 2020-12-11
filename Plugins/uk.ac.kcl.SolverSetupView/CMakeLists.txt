project(uk_ac_kcl_SolverSetupView)

mitk_create_plugin(
  EXPORT_DIRECTIVE SOLVERSETUP_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkSegmentation MitkSegmentationUI MitkPlanarFigure VesselTree SolidKernel ${CRIMSON_MESHING_KERNEL}MeshingKernel SolverSetupService PythonSolverSetupService CRIMSONUtils PCMRIKernel CRIMSONParticles
  PACKAGE_DEPENDS GSL VTK CTK Boost WM5 QtPropertyBrowser
)
