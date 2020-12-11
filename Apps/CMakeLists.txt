# note: MY_APP_NAME is always CRIMSON
message("Entering Apps/CMakeLists.txt")
if(${MY_PROJECT_NAME}_Apps/${MY_APP_NAME} OR ${MY_PROJECT_NAME}_BUILD_ALL_APPS)
  add_subdirectory(${MY_APP_NAME})
endif()

add_subdirectory(NetlistEditor)
message("End of Apps/CMakeLists.txt")