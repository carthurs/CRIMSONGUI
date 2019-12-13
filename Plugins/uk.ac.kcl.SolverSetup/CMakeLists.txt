project(uk_ac_kcl_SolverSetup)

mitk_create_plugin(
  EXPORT_DIRECTIVE SOLVERSETUP_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt PythonSolverSetupService
  PACKAGE_DEPENDS GSL
)
