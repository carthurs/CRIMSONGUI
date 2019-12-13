project(uk_ac_kcl_SolverSetupPython)

mitk_create_plugin(
  EXPORT_DIRECTIVE SOLVERSETUPPYTHON_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt PythonSolverSetupService
  PACKAGE_DEPENDS GSL CTK QtPropertyBrowser 
)
