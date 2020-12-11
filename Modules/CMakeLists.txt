# Modules must be listed according to their dependencies
set(module_dirs
    CRIMSONParticles
    AsyncTask 
    CRIMSONUtils
    VesselTree
    SolidKernel
    ${CRIMSON_MESHING_KERNEL}MeshingKernel
    PCMRIKernel 
    SolverSetupService
    PythonSolverSetupService
)


#set(MITK_DEFAULT_SUBPROJECTS MITK-Modules)

foreach(module_dir ${module_dirs})
  add_subdirectory(${module_dir})
endforeach()