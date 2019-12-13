cmake_minimum_required(VERSION 2.8.4)

IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Build type")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
ENDIF()
option(WITH_DEFORMABLE "Use fortran code for deformable simulation" ON) 
option(BUILD_TESTING "Build tests for presolver" ON) 

set(PRESOLVER_SRCS supre.cxx helpers.cxx cvSolverIO.cxx supre-cmds.cxx cmd.cxx)
set(PRESOLVER_LIBS)

if (WITH_DEFORMABLE)
    project(presolver C CXX Fortran)

    # NSPCG
    add_subdirectory(3rdParty/nspcg)
    list(APPEND PRESOLVER_LIBS SolverFortran nspcg)

    # SPARSE
    set(SPARSE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdParty/sparse/src)
    add_subdirectory(${SPARSE_SRC_DIR})
    list(APPEND PRESOLVER_LIBS SolverFortran sparse)
    include_directories(${SPARSE_SRC_DIR})

    include(CMakeAddFortranSubdirectory)
    cmake_add_fortran_subdirectory(Fortran NO_EXTERNAL_INSTALL)

    list(APPEND PRESOLVER_SRCS displacements.cxx directsolve.c)

    add_definitions(-DWITH_DEFORMABLE)
else()
    project(presolver C CXX)
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_executable(presolver ${PRESOLVER_SRCS})

target_link_libraries(presolver ${PRESOLVER_LIBS})
target_compile_features(presolver PRIVATE cxx_lambdas)

###
# Testing
###

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(Testing)
endif()

