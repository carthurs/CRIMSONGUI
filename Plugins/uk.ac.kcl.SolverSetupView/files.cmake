set(SRC_CPP_FILES
)

set(INTERNAL_CPP_FILES
  uk_ac_kcl_SolverSetupView_Activator.cpp
  SolverSetupView.cpp
  MeshAdaptView.cpp
  SolverStudyUIDsTableModel.cpp
  FaceDataEditorWidget.cpp
  MaterialVisualizationWidget.cpp
  ResliceView.cpp
  ThumbnailGenerator.cpp #
  ContourTypeConversion.cpp
  PCMRIUtils.cpp
  PCMRIMappingWidget.cpp
  MapAction.cpp
  TimeInterpolationDialog.cpp
  )
#TODO: only add files that are different than ones in VascularModeling plugin (try to export reusable ones from there)

set(UI_FILES
  src/internal/SolverSetupView.ui
  src/internal/BoundaryConditionTypeSelectionDialog.ui
  src/internal/MeshAdaptView.ui
  src/internal/FaceDataEditorWidget.ui
  src/internal/MaterialVisualizationWidget.ui
  src/internal/PCMRIMappingWidget.ui
  src/internal/ReslicePropertiesWidget.ui
  src/internal/createMaskedImageDialog.ui 
  src/internal/TimeInterpolationDialog.ui#
)

set(MOC_H_FILES
  src/internal/uk_ac_kcl_SolverSetupView_Activator.h
  src/internal/SolverSetupView.h
  src/internal/MeshAdaptView.h
  src/internal/SolverStudyUIDsTableModel.h
  src/internal/FaceDataEditorWidget.h
  src/internal/MaterialVisualizationWidget.h
  src/internal/ResliceView.h
  src/internal/PCMRIMappingWidget.h
  src/internal/MapAction.h
  src/internal/TimeInterpolationDialog.h
  src/internal/ThumbnailGenerator.h #
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/Icon_SolverSetupView.png 
  resources/Icon_MeshAdaptView.png
  resources/Icon_PCMRIMappingWidget.png
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
    resources/SolverSetup.qrc
	resources/vesselSegmentation.qrc
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

set(H_FILES ${MOC_H_FILES})
