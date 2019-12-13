project(uk_ac_kcl_VascularModeling)

mitk_create_plugin(
  EXPORT_DIRECTIVE VASCULARMODELING_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkSegmentation MitkSegmentationUI MitkPlanarFigure VesselTree SolidKernel
  PACKAGE_DEPENDS GSL VTK CTK Boost WM5
)
