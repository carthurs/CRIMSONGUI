find_package(QtPropertyBrowser REQUIRED)

list(APPEND ALL_INCLUDE_DIRECTORIES ${QtPropertyBrowser_INCLUDE_DIRS})
list(APPEND ALL_LIBRARIES ${QtPropertyBrowser_LIBRARIES})
#list(APPEND ALL_LIBRARY_DIRS ${PROPERTYEDITOR_LINK_DIRECTORY})

add_definitions(-DQT_QTPROPERTYBROWSER_IMPORT)
