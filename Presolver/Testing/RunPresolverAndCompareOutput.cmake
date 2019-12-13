if (NOT PresolverExecutable)
    message(FATAL_ERROR "PresolverExecutable not defined")
endif()

if (NOT ReferenceOutputFolder)
    message(FATAL_ERROR "ReferenceOutputFolder not defined")
endif()


execute_process(
    COMMAND ${PresolverExecutable} "the.supre" 
    RESULT_VARIABLE test_not_successful
    )

if(test_not_successful)
   message( SEND_ERROR "presolver execution failed" )
endif()


foreach (fileName IN ITEMS geombc.dat.1 restart.0.1)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E compare_files ${fileName} ${ReferenceOutputFolder}/${fileName}
        RESULT_VARIABLE test_not_successful
        )

    if(test_not_successful)
       message( SEND_ERROR "${fileName} does not match ${ReferenceOutputFolder}/${fileName}" )
    endif()
endforeach()

