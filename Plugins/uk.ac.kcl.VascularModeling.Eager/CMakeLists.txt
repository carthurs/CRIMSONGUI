project(uk_ac_kcl_VascularModeling_Eager)

mitk_create_plugin(
  EXPORT_DIRECTIVE VASCULARMODELINGEAGER_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkPlanarFigure VesselTree SolidKernel #MitkContourModel MitkSegmentation MitkSegmentationUI 
  PACKAGE_DEPENDS GSL
)
