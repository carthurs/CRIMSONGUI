cmake_minimum_required(VERSION 2.9)

project(ep-superbuild)

include(ExternalProject)

ExternalProject_Add(
    ep_src
    URL d:/1/ep/ep.zip
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(ep_src source_dir)
ExternalProject_Get_Property(ep_src binary_dir)
ExternalProject_Get_Property(ep_src install_dir)

macro(ONE_CONFIGURATION_BUILD_COMMAND out_var Config Step)
set(${out_var}
    ${CMAKE_COMMAND}
      -DDESIRED_CONFIGURATION=${Config}
      -DSTEP=${Step}
      -DBINARY_DIR=<BINARY_DIR>
      -DCMAKE_CFG_INTDIR=${CMAKE_CFG_INTDIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/BuildOneConfiguration.cmake)
endmacro()

macro(Add_OneConfiguration_ExternalProject Config)
    ONE_CONFIGURATION_BUILD_COMMAND(_build_command ${Config} BUILD)
    ONE_CONFIGURATION_BUILD_COMMAND(_install_command ${Config} INSTALL)

    ExternalProject_Add(
        ep_${Config}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${source_dir}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}
        BUILD_COMMAND ${_build_command}
        INSTALL_COMMAND ${_install_command}
        DEPENDS ep_src
          )
endmacro()

Add_OneConfiguration_ExternalProject(Debug)
Add_OneConfiguration_ExternalProject(Release)
