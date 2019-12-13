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


set(args --build ${BINARY_DIR} --config ${CMAKE_CFG_INTDIR})
if(STEP STREQUAL "INSTALL")
    list(APPEND args --target install)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} ${args} RESULT_VARIABLE error_code)

if(error_code)
  message(FATAL_ERROR "Build failed at step ${STEP}")
endif()
