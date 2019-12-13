set(SRC_CPP_FILES
    PythonSolverSetupPreferencePage.cpp
)

set(INTERNAL_CPP_FILES
  uk_ac_kcl_SolverSetupPython_Activator.cpp
)

set(UI_FILES
  src/PythonSolverSetupPreferencePage.ui
)

set(MOC_H_FILES
  src/internal/uk_ac_kcl_SolverSetupPython_Activator.h
  src/PythonSolverSetupPreferencePage.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

set(H_FILES ${MOC_H_FILES})
