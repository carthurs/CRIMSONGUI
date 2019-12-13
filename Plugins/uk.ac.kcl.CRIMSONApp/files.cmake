set(SRC_CPP_FILES
  CRIMSONApplication.cpp 
  CRIMSONAppWorkbenchAdvisor.cpp 
  CRIMSONAppWorkbenchWindowAdvisor.cpp 
  CRIMSONWorkflowWidget.cpp
  ExpandablePerspectiveButton.cpp
  PopupWidget.cpp
)

set(INTERNAL_CPP_FILES
  uk_ac_kcl_CRIMSONApp_Activator.cpp
  Perspectives/GeometryModelingPerspective.cpp
  Perspectives/MeshingAndSolverSetupPerspective.cpp
)

set(UI_FILES
)

set(MOC_H_FILES
  src/internal/uk_ac_kcl_CRIMSONApp_Activator.h
  src/CRIMSONApplication.h
  src/CRIMSONAppWorkbenchAdvisor.h
  src/CRIMSONAppWorkbenchWindowAdvisor.h
  src/CRIMSONWorkflowWidget.h
  src/ExpandablePerspectiveButton.h
  src/PopupWidget_p.h
  src/PopupWidget.h
  src/internal/Perspectives/GeometryModelingPerspective.h
  src/internal/Perspectives/MeshingAndSolverSetupPerspective.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  plugin.xml
  resources/Icon_VesselBlendingView.png
  resources/Icon_MeshingAndSolverSetupPerspective.png
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
  resources/vmStyle.qrc
  resources/logo.qrc
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

set(H_FILES ${MOC_H_FILES})