cmake_minimum_required(VERSION 2.8.4)

set(SOLVER_F_SRCS stannspcg.f)

foreach(SRC SOLVER_F_SRCS)
    set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS -cpp) # Run fortran files through cpp preprocessor   
endforeach()

if (NOT ${SV_MAKE_WITH_LESLIB_VER_1_4})
	list(APPEND SOLVER_F_SRCS ramg_driver.f)
endif()

add_library(SolverFortran ${SOLVER_F_SRCS})

set_target_properties(SolverFortran PROPERTIES LINKER_LANGUAGE CXX)
