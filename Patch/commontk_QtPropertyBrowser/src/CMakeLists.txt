add_definitions(-DQT_QTPROPERTYBROWSER_EXPORT)

SET(KIT_SRCS
  qtbuttonpropertybrowser.cpp
  qtbuttonpropertybrowser.h
  qteditorfactory.cpp
  qteditorfactory.h
  qtgroupboxpropertybrowser.cpp
  qtgroupboxpropertybrowser.h
  qtpropertybrowser.cpp
  qtpropertybrowser.h
  qtpropertybrowserutils.cpp
  qtpropertybrowserutils_p.h
  qtpropertymanager.cpp
  qtpropertymanager.h
  qttreepropertybrowser.cpp
  qttreepropertybrowser.h
  qtvariantproperty.cpp
  qtvariantproperty.h
  QDoubleSpinBoxG.cpp
  QDoubleSpinBoxG.h
  )

SET(KIT_MOC_SRCS
  qtpropertybrowserutils_p.h
  QDoubleSpinBoxG.h
  )

SET(KIT_UI_FORMS
  )

SET(KIT_resources
  qtpropertybrowser.qrc
  )
  
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
  
QT5_WRAP_UI(KIT_UI_CPP ${KIT_UI_FORMS})
#QT5_WRAP_CPP(KIT_MOC_CPP ${KIT_MOC_SRCS})
QT5_ADD_RESOURCES(KIT_QRC_SRCS ${KIT_resources})

SET(libname ${PROJECT_NAME})
ADD_LIBRARY(${libname} SHARED
  ${KIT_SRCS}
  ${KIT_UI_CPP}
#  ${KIT_MOC_CPP}
  ${KIT_QRC_SRCS}
  )
  
TARGET_LINK_LIBRARIES(
  ${libname}
  ${${PROJECT_NAME}_LINK_LIBRARIES}
  )

qt5_use_modules(${libname} Widgets Gui Core)
  