# - Find GSL
# Find the GSL includes

if (GSL_INCLUDE_DIR)
  # Already in cache, be silent
  set (GSL_FIND_QUIETLY TRUE)
endif (GSL_INCLUDE_DIR)

find_path(GSL_INCLUDE_DIR gsl.h CACHE STRING DOC "GSL include directory")

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GSL DEFAULT_MSG GSL_INCLUDE_DIR)