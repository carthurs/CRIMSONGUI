string(REPLACE "\"" "" MITK_DIR_NO_QUOTES ${MITK_DIR})

include("${MITK_DIR_NO_QUOTES}/MITKPython.cmake")

function(crimsonFunctionExternalPythonBuildStep proj step _bin_dir)

  # the specific python build command run by this step
  set(_command ${ARGN})

  message("Running ${proj} ${step}: ${PYTHON_EXECUTABLE} ${_command}")

  set(_workdir "${_bin_dir}")
  set(_python "${PYTHON_EXECUTABLE}")

  # escape spaces
  if(UNIX)
    STRING(REPLACE " " "\ " _workdir ${_workdir})
    STRING(REPLACE " " "\ " _python ${_python})
  endif()

  execute_process(
     COMMAND ${_python} ${_command}
     WORKING_DIRECTORY ${_workdir}
     RESULT_VARIABLE result
     #ERROR_QUIET
     ERROR_VARIABLE error
     OUTPUT_VARIABLE output
     #OUTPUT_QUIET
  )

  if(NOT ${result} EQUAL 0)
    message("Error in: ${proj}: ${error}")
    message("Output in: ${proj}: ${output}")
  endif()
endfunction()

