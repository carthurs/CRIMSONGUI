# - Try to find SPARSE library
# Once done this will define
#  SPARSE_FOUND - System has SPARSE 
#  SPARSE_INCLUDE_DIR - The SPARSE include directories
#  SPARSE_LIBRARIES - The libraries needed to use SPARSE

include(LibFindMacros)

set(SPARSE_ROOT "" CACHE PATH "Path to the Sparse library root folder") 
if (NOT SPARSE_ROOT)
    message(FATAL_ERROR "Please set SPARSE_ROOT to find the SPARSE library.")
else()
    find_path(SPARSE_INCLUDE_DIR "spDefs.h" PATHS ${SPARSE_ROOT} "${SPARSE_ROOT}/include" "${SPARSE_ROOT}/src" DOC "Sparse library include path.")
    find_library(SPARSE_LIBRARY NAMES libsparse sparse PATHS ${SPARSE_ROOT} DOC "Sparse library")
endif()

set(SPARSE_PROCESS_INCLUDES SPARSE_INCLUDE_DIR)
set(SPARSE_PROCESS_LIBS SPARSE_LIBRARY)
libfind_process(SPARSE)