set(SRC_CPP_FILES
    FaceListTableModel.cpp
    NodeDependentView.cpp
    SelectFacesFromListDialog.cpp
    SelectedFacesTableModel.cpp
)

set(INTERNAL_CPP_FILES
  uk_ac_kcl_VascularModeling_Activator.cpp
  VesselPathPlanningView.cpp
  vesselPathItemModel.cpp
  vesselPathTableViewWidget.cpp
  VesselDrivenResliceView.cpp
  ContourModelingView.cpp
  VesselBlendingView.cpp
  BooleanOperationItemDelegate.cpp
  #HierarchyManager.cpp
  ThumbnailGenerator.cpp
  ContourTypeConversion.cpp
  VascularModelingUtils.cpp
  LoftAction.cpp
  BlendAction.cpp
  ReparentAction.cpp
  ShowHideContoursAction.cpp
  ExportVesselsAction.cpp
  ImportVesselsAction.cpp
  3rdParty/BestFit/BestFit.cpp
  3rdParty/BestFit/Double.cpp
  3rdParty/BestFit/Shapes.cpp
)

set(UI_FILES
  src/internal/vesselSegmentationToolbox.ui
  src/internal/ContourModelingView.ui
  src/internal/VesselBlendingView.ui
  src/internal/ReslicePropertiesWidget.ui
  src/internal/createMaskedImageDialog.ui
  src/internal/UseVesselsInBlendingDialog.ui
  src/internal/ReparentDialog.ui
  src/SelectFacesFromListDialog.ui
)

set(MOC_H_FILES
  src/internal/uk_ac_kcl_VascularModeling_Activator.h
  src/internal/VesselPathPlanningView.h
  src/internal/vesselPathItemModel.h
  src/internal/vesselPathTableViewWidget.h
  src/internal/VesselDrivenResliceView.h
  src/internal/ContourModelingView.h
  src/internal/VesselBlendingView.h
  src/internal/BooleanOperationItemDelegate.h
  src/internal/ThumbnailGenerator.h
  src/internal/UseVesselsInBlendingDialog.h
  src/internal/LoftAction.h
  src/internal/BlendAction.h
  src/internal/ReparentAction.h
  src/internal/ShowHideContoursAction.h
  src/internal/ExportVesselsAction.h
  src/internal/ImportVesselsAction.h
  src/internal/DetectIntersectionsTask.h
  src/FaceListTableModel.h
  src/SelectedFacesTableModel.h
  src/SelectFacesFromListDialog.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/Icon_VesselDrivenResliceView.png
  resources/Icon_ContourModelingView.png
  resources/Icon_VesselTreeView.png
  resources/Icon_VesselBlendingView.png
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
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