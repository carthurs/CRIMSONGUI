file(GLOB_RECURSE H_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

set(CPP_FILES
  SolutionData.cpp
  IO/SolutionDataIO.cpp
  IO/SolverSetupServiceIOMimeTypes.cpp
  Initialization/SolverSetupServiceModuleActivator.cpp
)

set(RESOURCE_FILES
  Interactions/PointSetConfigCRIMSON.xml
)