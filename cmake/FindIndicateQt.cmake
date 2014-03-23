# CMake wrapper for finding indicate-qt
# This is not very flexible (e.g. no version check), but allows using
# a normal find_package call and thus support for feature_summary
#
# (C) 2014 by the Quassel Project <devel@quassel-irc.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Once found, sets the standard set of variables:
# INDICATEQT_FOUND - IndicateQt available in the system
# INDICATEQT_LIBRARIES - Libraries to link with
# INDICATEQT_INCLUDE_DIRS - Include directories containing the headers
#
##############################################################################

include(FindPackageHandleStandardArgs)

if (USE_QT4)
    # requires PkgConfig for now; patches for finding it directly welcome!
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(INDICATEQT QUIET indicate-qt>=0.2.1)
    endif()
endif()

find_package_handle_standard_args(IndicateQt DEFAULT_MSG INDICATEQT_LIBRARIES INDICATEQT_INCLUDE_DIRS)
mark_as_advanced(INDICATEQT_LIBRARIES INDICATEQT_INCLUDE_DIRS)
