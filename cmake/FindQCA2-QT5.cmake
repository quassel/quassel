# - Try to find QCA2 (Qt Cryptography Architecture 2) for QT5
# Once done this will define
#
#  QCA2-QT5_FOUND - system has QCA2-QT5
#  QCA2-QT5_INCLUDE_DIR - the QCA2-QT5 include directory
#  QCA2-QT5_LIBRARIES - the libraries needed to use QCA2-QT5
#  QCA2-QT5_DEFINITIONS - Compiler switches required for using QCA2-QT5
#
# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

# Copyright (c) 2006, Michael Larouche, <michael.larouche@kdemail.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

find_package(Qca-qt5 QUIET)
if(Qca-qt5_FOUND)

  set(QCA2-QT5_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)#just to have any value as the real include dir is imported by linking to qca-qt5
  set(QCA2-QT5_LIBRARIES qca-qt5)

  find_package_handle_standard_args(QCA2-QT5 DEFAULT_MSG QCA2-QT5_LIBRARIES QCA2-QT5_INCLUDE_DIR)

  mark_as_advanced(QCA2-QT5_INCLUDE_DIR QCA2-QT5_LIBRARIES)
else()

include(FindLibraryWithDebug)

if (QCA2-QT5_INCLUDE_DIR AND QCA2-QT5_LIBRARIES)

  # in cache already
  set(QCA2-QT5_FOUND TRUE)

else (QCA2-QT5_INCLUDE_DIR AND QCA2-QT5_LIBRARIES)


  if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_QCA2-QT5 QUIET qca2-qt5)
    set(QCA2-QT5_DEFINITIONS ${PC_QCA2-QT5_CFLAGS_OTHER})
  endif (NOT WIN32)

  find_library_with_debug(QCA2-QT5_LIBRARIES
                  WIN32_DEBUG_POSTFIX d
                  NAMES qca-qt5
                  HINTS ${PC_QCA2-QT5_LIBDIR} ${PC_QCA2-QT5_LIBRARY_DIRS}
                  )

  find_path(QCA2-QT5_INCLUDE_DIR QtCrypto
            HINTS ${PC_QCA2-QT5_INCLUDEDIR} ${PC_QCA2-QT5_INCLUDE_DIRS}
            PATH_SUFFIXES QtCrypto)

  find_package_handle_standard_args(QCA2-QT5 DEFAULT_MSG QCA2-QT5_LIBRARIES QCA2-QT5_INCLUDE_DIR)

  mark_as_advanced(QCA2-QT5_INCLUDE_DIR QCA2-QT5_LIBRARIES)

endif (QCA2-QT5_INCLUDE_DIR AND QCA2-QT5_LIBRARIES)
endif()
