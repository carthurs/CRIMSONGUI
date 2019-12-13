# - Find Mach7
# Find the Mach7 includes
#
#  Mach7_INCLUDE_DIR    - where to find fftw3.h

if (Mach7_INCLUDE_DIR)
  # Already in cache, be silent
  set (Mach7_FIND_QUIETLY TRUE)
endif (Mach7_INCLUDE_DIR)

find_path(Mach7_INCLUDE_DIR match.hpp CACHE STRING DOC "Mach7 include directory")

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Mach7 DEFAULT_MSG Mach7_INCLUDE_DIR)