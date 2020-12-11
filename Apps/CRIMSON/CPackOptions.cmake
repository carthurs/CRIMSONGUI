# Set CRIMSON specific CPack options

set(CPACK_PACKAGE_EXECUTABLES "CRIMSON")
set(CPACK_PACKAGE_NAME "CRIMSON")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cardiovascular Integrated Modelling & Simulation")
# Major version is the year of release
set(CPACK_PACKAGE_VERSION_MAJOR ${CRIMSON_VERSION_MAJOR})
# Minor version is the month of release
set(CPACK_PACKAGE_VERSION_MINOR ${CRIMSON_VERSION_MINOR})
# Patch versioning is number of hotfix patch
set(CPACK_PACKAGE_VERSION_PATCH ${CRIMSON_VERSION_PATCH})

set(CRIMSON_TRIAL_POSTFIX)
if (CRIMSON_BUILD_TRIAL_VERSION)
    set(CRIMSON_TRIAL_POSTFIX "-trial")
endif()

# this should result in names like 2011.09, 2012.06, ...
# version has to be set explicitly to avoid such things as CMake creating the install directory "MITK Diffusion 2011.."
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_PACKAGE_FILE_NAME "CRIMSON-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_ARCH}${CRIMSON_TRIAL_POSTFIX}")

