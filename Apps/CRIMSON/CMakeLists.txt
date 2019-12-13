project(CRIMSON)

set(_app_options)
if(${MY_PROJECT_NAME}_SHOW_CONSOLE_WINDOW)
  list(APPEND _app_options SHOW_CONSOLE)
endif()

#mitkFunctionCreateWindowsBatchScript(JenkinsEnv.properties.in JenkinsEnv.properties release)

# Create a cache entry for the provisioning file which is used to export
# the file name in the MITKConfig.cmake file. This will keep external projects
# which rely on this file happy.
set(MITK_EXTAPP_PROVISIONING_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CRIMSON.provisioning" CACHE INTERNAL "CRIMSON provisioning file" FORCE)


# Plug-ins listed below will not be
#  - added as a build-time dependency to the executable
#  - listed in the provisioning file for the executable
#  - installed if they are external plug-ins

set(_exclude_plugins

)

QT5_ADD_RESOURCES(qrc_files splashscreen.qrc)

crimsonFunctionCreateBlueBerryApplication(
  NAME ${MY_APP_NAME}
  SOURCES ${qrc_files} CRIMSON.cpp
  DESCRIPTION "${MY_APP_NAME} Application"
  EXCLUDE_PLUGINS ${_exclude_plugins}
  ${_app_options}
)
mitk_use_modules(TARGET ${MY_APP_NAME} MODULES MitkAppUtil)

install(FILES "${CMAKE_CURRENT_LIST_DIR}/icons/icon.ico" DESTINATION bin)

include(DeployQt5)
install_qt5_executable("bin/${MY_APP_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${Qt5Gui_PLUGINS};${Qt5Sql_PLUGINS};${Qt5Svg_PLUGINS}" "" "" "plugins" TRUE)




