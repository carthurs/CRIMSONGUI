project(uk_ac_kcl_AsyncTaskManager)

mitk_create_plugin(
  EXPORT_DIRECTIVE ASYNCTASKMANAGER_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS AsyncTask  
  PACKAGE_DEPENDS Qt5|Widgets GSL
)
