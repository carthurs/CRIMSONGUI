macro(AddPresolverTest TestName TestDataFolderName)
    add_test( NAME ${TestName}
       COMMAND ${CMAKE_COMMAND} 
       -DPresolverExecutable=$<TARGET_FILE:presolver>
       -DReferenceOutputFolder=${CMAKE_CURRENT_SOURCE_DIR}/${TestDataFolderName}/ReferenceOutput
       -P ${CMAKE_CURRENT_SOURCE_DIR}/RunPresolverAndCompareOutput.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${TestDataFolderName}/Input # Input 
       )
endmacro()

AddPresolverTest("testNonDeformable" "Rigid")
   
if (WITH_DEFORMABLE)
    AddPresolverTest("testDeformable" "Deformable")
endif()

