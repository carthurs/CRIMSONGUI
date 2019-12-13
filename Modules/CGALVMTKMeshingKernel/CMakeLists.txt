MITK_CREATE_MODULE( CGALVMTKMeshingKernel 
  INCLUDE_DIRS DataManagement ExtensionPoint IO Rendering
  DEPENDS MitkSceneSerializationBase MitkMapperExt SolidKernel
  PACKAGE_DEPENDS PUBLIC GSL OCC Boost VMTK VTK|vtkFiltersVerdict CGAL Qt5|Widgets+Core
  PRIVATE Eigen3 WM5
)

target_compile_definitions(${MODULE_TARGET} PRIVATE TETLIBRARY)

IF( BUILD_TESTING )
#  add_subdirectory(Testing)
ENDIF()
