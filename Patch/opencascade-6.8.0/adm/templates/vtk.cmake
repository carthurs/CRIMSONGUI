# vtk

if (NOT DEFINED INSTALL_VTK)
  set (INSTALL_VTK OFF CACHE BOOL "Is vtk required to be copied into install directory")
endif()

# vtk directory
if (NOT DEFINED 3RDPARTY_VTK_DIR)
  set (3RDPARTY_VTK_DIR "" CACHE PATH "The directory containing vtk")
endif()

# vtk include directory
if (NOT DEFINED 3RDPARTY_VTK_INCLUDE_DIR)
  set (3RDPARTY_VTK_INCLUDE_DIR "" CACHE FILEPATH "The directory containing headers of vtk")
endif()

# vtk library directory
if (NOT DEFINED 3RDPARTY_VTK_LIBRARY_DIR)
  set (3RDPARTY_VTK_LIBRARY_DIR "" CACHE FILEPATH "The directory containing vtk library")
endif()

# vtk dll directory
if (WIN32 AND NOT DEFINED 3RDPARTY_VTK_DLL_DIR)
  set (3RDPARTY_VTK_DLL_DIR "" CACHE FILEPATH "The directory containing VTK dll")
endif()

# search for vtk in user defined directory
if (NOT 3RDPARTY_VTK_DIR AND 3RDPARTY_DIR)
  FIND_PRODUCT_DIR("${3RDPARTY_DIR}" vtk VTK_DIR_NAME)
  if (VTK_DIR_NAME)
    set (3RDPARTY_VTK_DIR "${3RDPARTY_DIR}/${VTK_DIR_NAME}" CACHE PATH "The directory containing vtk product" FORCE)
  endif()
endif()

# find installed vtk
find_package(VTK QUIET)

# find native vtk
if (NOT VTK_FOUND)
  find_package(VTK QUIET PATHS "${3RDPARTY_VTK_DIR}")
endif()

include(${VTK_USE_FILE})

message(WARNING ${VTK_LIBRARY_DIRS})

set(3RDPARTY_VTK_DIR ${VTK_DIR})
set(3RDPARTY_LIBRARY_DIRS ${3RDPARTY_LIBRARY_DIRS} ${VTK_LIBRARY_DIRS})
set(3RDPARTY_INCLUDE_DIRS ${3RDPARTY_INCLUDE_DIRS} ${VTK_INCLUDE_DIRS})

# vtk libraries
# lib
set (VTK_LIBRARY_NAMES ${VTK_LIBRARIES})

#dll
set (VTK_DLL_NAMES vtkCommonComputationalGeometry-${VTK_VERSION}.dll
                   vtkCommonCore-${VTK_VERSION}.dll
                   vtkCommonDataModel-${VTK_VERSION}.dll
                   vtkCommonExecutionModel-${VTK_VERSION}.dll
                   vtkCommonMath-${VTK_VERSION}.dll
                   vtkCommonMisc-${VTK_VERSION}.dll
                   vtkCommonSystem-${VTK_VERSION}.dll
                   vtkCommonTransforms-${VTK_VERSION}.dll
                   vtkDICOMParser-${VTK_VERSION}.dll
                   vtkFiltersCore-${VTK_VERSION}.dll
                   vtkFiltersExtraction-${VTK_VERSION}.dll
                   vtkFiltersGeneral-${VTK_VERSION}.dll
                   vtkFiltersGeometry-${VTK_VERSION}.dll
                   vtkFiltersSources-${VTK_VERSION}.dll
                   vtkFiltersStatistics-${VTK_VERSION}.dll
                   vtkIOCore-${VTK_VERSION}.dll
                   vtkIOImage-${VTK_VERSION}.dll
                   vtkImagingCore-${VTK_VERSION}.dll
                   vtkImagingFourier-${VTK_VERSION}.dll
                   vtkImagingHybrid-${VTK_VERSION}.dll
                   vtkInteractionStyle-${VTK_VERSION}.dll
                   vtkRenderingCore-${VTK_VERSION}.dll
                   vtkRenderingOpenGL-${VTK_VERSION}.dll
                   vtkalglib-${VTK_VERSION}.dll
                   vtkjpeg-${VTK_VERSION}.dll
                   vtkmetaio-${VTK_VERSION}.dll
                   vtkpng-${VTK_VERSION}.dll
                   vtksys-${VTK_VERSION}.dll
                   vtktiff-${VTK_VERSION}.dll
                   vtkzlib-${VTK_VERSION}.dll )

# search for dll directory
if (WIN32)
  if (NOT 3RDPARTY_VTK_DLL_DIR OR NOT EXISTS "${3RDPARTY_VTK_DLL_DIR}")
    if(EXISTS "${3RDPARTY_VTK_DIR}/bin${BUILD_POSTFIX}")
      set (3RDPARTY_VTK_DLL_DIR "${3RDPARTY_VTK_DIR}/bin${BUILD_POSTFIX}" CACHE FILEPATH "The directory containing dll of VTK" FORCE)
    else()
	  if (NOT "${BUILD_POSTFIX}" STREQUAL "" AND EXISTS "${3RDPARTY_VTK_DIR}/bin")
	    set (3RDPARTY_VTK_DLL_DIR "${3RDPARTY_VTK_DIR}/bin" CACHE FILEPATH "The directory containing dll of VTK" FORCE)
      endif()
	endif()
  endif()
endif() 

OCCT_CHECK_AND_UNSET(VTK_DIR)

mark_as_advanced (VTK_INCLUDE_DIRS VTK_LIBRARY_DIRS VTK_DIR)
