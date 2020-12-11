
# This file is included in the top-level CMakeLists.txt file to
# allow early dependency checking

# https://stackoverflow.com/questions/36358217/what-is-the-difference-between-option-and-set-cache-bool-for-a-cmake-variabl
# note: option is not really all that different from a cache variable
option(${MY_PROJECT_NAME}_Apps/${MY_APP_NAME} "Build the ${MY_APP_NAME}" ON)


# This variable is fed to ctkFunctionSetupPlugins() macro in the
# top-level CMakeLists.txt file. This allows to automatically
# enable required plug-in runtime dependencies for applications using
# the CTK DGraph executable and the ctkMacroValidateBuildOptions macro.
# For this to work, directories containing executables must contain
# a CMakeLists.txt file containing a "project(...)" command and a
# target_libraries.cmake file setting a list named "target_libraries"
# with required plug-in target names.

# [AJM] It looks like in SuperBuild.cmake, '^^' was set as a placeholder for ;, though I am not sure what a semicolon would be doing here.
#       As far as I can tell, ^^ doesn't have any reserved meaning in cmake.
set(PROJECT_APPS
  Apps/${MY_APP_NAME}^^${MY_PROJECT_NAME}_Apps/${MY_APP_NAME}
)