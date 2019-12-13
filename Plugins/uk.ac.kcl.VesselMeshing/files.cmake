set(SRC_CPP_FILES
)

set(INTERNAL_CPP_FILES
  uk_ac_kcl_VesselMeshing_Activator.cpp
  VesselMeshingView.cpp
  MeshExplorationView.cpp
  MeshInformationDialog.cpp
  MeshAction.cpp
  MeshingUtils.cpp
  ShowMeshInformationAction.cpp
  ConvertToDiscreteModelAction.cpp
)

set(UI_FILES
  src/internal/MeshExplorationView.ui
  src/internal/VesselMeshingView.ui
  src/internal/MeshInformationDialog.ui
)

set(MOC_H_FILES
  src/internal/uk_ac_kcl_VesselMeshing_Activator.h
  src/internal/VesselMeshingView.h
  src/internal/MeshExplorationView.h
  src/internal/MeshInformationDialog.h
  src/internal/MeshAction.h
  src/internal/ShowMeshInformationAction.h
  src/internal/ConvertToDiscreteModelAction.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/Icon_VesselMeshingView.png
  resources/Icon_MeshExplorationView.png
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
    resources/VesselMeshing.qrc
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

set(H_FILES ${MOC_H_FILES})
