message("Start of Modules/CGALVMTKMeshingKernel/CMakeLists.txt")

MITK_CREATE_MODULE( CGALVMTKMeshingKernel 
  INCLUDE_DIRS DataManagement ExtensionPoint IO Rendering
  DEPENDS MitkSceneSerializationBase MitkMapperExt SolidKernel
  PACKAGE_DEPENDS PUBLIC GSL OCC Boost VMTK VTK|vtkFiltersVerdict CGAL Qt5|Widgets+Core
  PRIVATE Eigen3 WM5
)

# [AJM] Debug builds require this, to avoid this error:
#   error C1128: number of sections exceeded object file format limit : 
#   compile with /bigobj [C:\fd\CRIMSON-build\Modules\CGALVMTKMeshingKernel\CGALVMTKMeshingKernel.vcxproj]
#   C:\tetcrim\Modules\CGALVMTKMeshingKernel\ExtensionPoint\IMeshingKernel.cpp 1 1 CRIMSON-build

if (MSVC)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("MSVC Debug build detected: Enabling /bigobj")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj" )
  endif()
endif()

target_compile_definitions(${MODULE_TARGET} PRIVATE TETLIBRARY)

message("End of Modules/CGALVMTKMeshingKernel/CMakeLists.txt")