file(GLOB_RECURSE H_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

set(CPP_FILES
    PythonBoundaryCondition.cpp
    PythonBoundaryConditionSet.cpp
    PythonSolverParametersData.cpp
    PythonSolverSetupManager.cpp
    PythonSolverSetupServiceActivator.cpp
    PythonSolverStudyData.cpp
    PythonSolverSetupService.cpp
    PythonMaterialData.cpp
    PythonFaceDataImpl.cpp
)

set(MOC_H_FILES
    src/PythonQtWrappers.h
)
