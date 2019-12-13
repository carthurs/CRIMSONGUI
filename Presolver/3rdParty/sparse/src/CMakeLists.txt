cmake_minimum_required(VERSION 2.8.4)

add_library(sparse spAllocate.c spBuild.c spFactor.c spFortran.c spOutput.c spSolve.c spUtils.c)
set_target_properties(nspcg PROPERTIES LINKER_LANGUAGE C)

