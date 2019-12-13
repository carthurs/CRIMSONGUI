project(uk_ac_kcl_VesselMeshing)

mitk_create_plugin(
  EXPORT_DIRECTIVE VESSELMESHING_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt SolidKernel ${CRIMSON_MESHING_KERNEL}MeshingKernel
  PACKAGE_DEPENDS GSL VTK CTK Boost
)
