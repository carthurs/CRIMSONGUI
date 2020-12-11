#[AJM] this file appears to be only used for OCC.
# Inputs: 
#   DESIRED_CONFIGURATION
#   STEP
#   BINARY_DIR
#     In our case, C:/cr/CMakeExternals/Build/OCC_Release
#   CMAKE_CFG_INTDIR
#     https://cmake.org/cmake/help/v3.13/variable/CMAKE_CFG_INTDIR.html
#     Inherited from caller, literally just $(CONFIGURATION) for visual studio
message("Entering ./CMakeExternals/BuildOneConfiguration.cmake")
if (NOT DEFINED DESIRED_CONFIGURATION)
  message(FATAL_ERROR "DESIRED_CONFIGURATION must be defined.")
endif()

if (NOT DEFINED STEP)
  message(FATAL_ERROR "STEP must be defined.")
endif()

if (NOT DEFINED BINARY_DIR)
  message(FATAL_ERROR "BINARY_DIR must be defined.")
endif()

#message(INFO "Step: ${STEP}, Target: ${CMAKE_CFG_INTDIR}, Desired target: ${DESIRED_CONFIGURATION}"

if (NOT CMAKE_CFG_INTDIR STREQUAL DESIRED_CONFIGURATION)
    return()
endif()

message("BINARY_DIR=" ${BINARY_DIR})
set(args --build ${BINARY_DIR} --config ${CMAKE_CFG_INTDIR})
if(STEP STREQUAL "INSTALL")
    #I think this literally means, build a target named "install";
    #OCC_Release does have a VSProj by that name in 
    #C:\cr\CMakeExternals\Build\OCC_Release
    list(APPEND args --target install)
endif()

# Ends up being something like 
# cmake --build BINARY_DIR --config DESIRED_CONFIGURATION [--target install]
execute_process(COMMAND ${CMAKE_COMMAND} ${args} RESULT_VARIABLE error_code)

if(error_code)
  message(FATAL_ERROR "Build failed at step ${STEP}")
endif()
message("End of ./CMakeExternals/BuildOneConfiguration.cmake")