cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_Fortran_CREATE_STATIC_LIBRARY ON)

add_library(nspcg nspcg1.f nspcg2.f nspcg3.f nspcg4.f nspcg5.f)
set_target_properties(nspcg PROPERTIES LINKER_LANGUAGE Fortran)
