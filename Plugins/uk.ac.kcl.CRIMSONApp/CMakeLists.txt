project(uk_ac_kcl_CRIMSONApp)

configure_file(src/CRIMSONVersion.h.in ${CMAKE_CURRENT_LIST_DIR}/src/CRIMSONVersion.h)

mitk_create_plugin(
  EXPORT_DIRECTIVE CRIMSONAPP_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt
  PACKAGE_DEPENDS GSL   
)
