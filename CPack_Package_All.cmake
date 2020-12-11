#-----------------------------------------------------------------------------
# Superbuild CPack (temporarily disabled!)
# This script is not done yet. I had to move on to other things.
#-----------------------------------------------------------------------------

message(FATAL_ERROR "Attempted to run CPack_Package_All.cmake but this script is not finished yet.")

# ----------------------------------------------------------------------------------------------------------------------
# Read the "binary directories" of both external projects
#
# CMake calls the build\extern-prefix\src\extern-build a "Binary directory", but it's really the directory where the VS Solution is, etc.
# I'd call these "build directories", to be honest...
# ----------------------------------------------------------------------------------------------------------------------

unset(BINARY_DIR)

#This function writes to a variable named BUILD_DIR... okay sure?
#It'll get clobbered twice so we have to save the result somewhere else.....
ExternalProject_Get_property(QtPropertyBrowser BINARY_DIR) 
set(QtPropertyBrowserBuildDirectory ${BINARY_DIR})
message("QtPropertyBrowser's build directory is " ${externBuildDirectory})

unset(BINARY_DIR)

ExternalProject_Get_property(OCC BINARY_DIR)
set(OCCBuildDirectory ${BINARY_DIR})
message("OCC's build directory is " ${mainBuildDirectory})

unset(BINARY_DIR)

# Write these paths to CPACK_INSTALL_CMAKE_PROJECTS so that CPack will know to visit these projects for packaging

# https://cmake.org/cmake/help/latest/module/CPack.html?highlight=cpack_install_cmake_projects#variable:CPACK_INSTALL_CMAKE_PROJECTS
#                                   project build dir                   proj name               build all components    "Directory" (??)
set(QtPropertyBrowserProjectInfo    ${QtPropertyBrowserBuildDirectory}  "QtPropertyBrowser"     "ALL"                   "/")
set(OCCProjectInfo                  ${OCCBuildDirectory}                "OCC"                   "ALL"                   "/")

# both of these go in one list
set(allProjectInfo ${QtPropertyBrowserProjectInfo} ${OCCProjectInfo})

message("CPACK_INSTALL_CMAKE_PROJECTS before append is " ${CPACK_INSTALL_CMAKE_PROJECTS})

# I think this variable has to be set before CPack is included.
set(CPACK_INSTALL_CMAKE_PROJECTS ${allProjectInfo})

message("CPACK_INSTALL_CMAKE_PROJECTS after append is " ${CPACK_INSTALL_CMAKE_PROJECTS})

# ----------------------------------------------------------------------------------------------------------------------
# Now include CPack (must be last)
# ----------------------------------------------------------------------------------------------------------------------

include(CPack)
set(CPACK_GENERATOR NSIS)

message("End of ./SuperBuild.cmake")