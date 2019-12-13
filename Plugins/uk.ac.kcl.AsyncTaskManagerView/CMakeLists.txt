project(uk_ac_kcl_AsyncTaskManagerView)

mitk_create_plugin(
  EXPORT_DIRECTIVE ASYNCTASKMANAGER_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS AsyncTask MitkQtWidgetsExt
  PACKAGE_DEPENDS GSL CTK # Annoying qRestAPI.lib dependency  
)
