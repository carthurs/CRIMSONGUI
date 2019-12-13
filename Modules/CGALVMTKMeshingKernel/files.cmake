set(CPP_FILES
    DataManagement/MeshData.cpp
    DataManagement/MeshingParametersData.cpp
    IO/MeshingParametersDataIO.cpp 
    IO/MeshDataCoreObjectFactory.cpp 
    IO/MeshDataIO.cpp 
    IO/MeshingKernelIOMimeTypes.cpp 
    ExtensionPoint/IMeshingKernel.cpp
    ExtensionPoint/crimsonTetGenWrapper.cpp
    ExtensionPoint/tetgen.cxx
    ExtensionPoint/predicates.cxx
    ExtensionPoint/ui/GlobalMeshingParametersWidget.cpp
    ExtensionPoint/ui/LocalMeshingParametersDialog.cpp
    ExtensionPoint/ui/MeshingKernelUI.cpp
    Rendering/MeshDataMapper.cpp
    Initialization/MeshingKernelModuleActivator.cpp
 )

set(MOC_H_FILES
  ExtensionPoint/ui/GlobalMeshingParametersWidget.h
  ExtensionPoint/ui/LocalMeshingParametersDialog.h
  ExtensionPoint/ui/LocalParameterEditor.h
)

set(UI_FILES
  ExtensionPoint/ui/GlobalMeshingParametersWidget.ui
  ExtensionPoint/ui/LocalMeshingParametersDialog.ui
)

set(QRC_FILES
  ExtensionPoint/ui/resources/MeshingKernelUI.qrc
)
