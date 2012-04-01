# Find libphonon
# Once done this will define
#
#  PHONON_FOUND    - system has Phonon Library
#  PHONON_INCLUDES - the Phonon include directory
#  PHONON_LIBS     - link these to use Phonon
#  PHONON_VERSION  - the version of the Phonon Library

# Copyright (c) 2008, Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(_phonon_find_version)
   set(_phonon_namespace_header_file "${PHONON_INCLUDE_DIR}/phonon/phononnamespace.h")
   if (APPLE AND EXISTS "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
      set(_phonon_namespace_header_file "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
   endif (APPLE AND EXISTS "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
   file(READ ${_phonon_namespace_header_file} _phonon_header LIMIT 5000 OFFSET 1000)
   string(REGEX MATCH "define PHONON_VERSION_STR \"(4\\.[0-9]+\\.[0-9a-z]+)\"" _phonon_version_match "${_phonon_header}")
   set(PHONON_VERSION "${CMAKE_MATCH_1}")
   message(STATUS "Phonon Version: ${PHONON_VERSION}")
endmacro(_phonon_find_version)

if(PHONON_FOUND)
   # Already found, nothing more to do except figuring out the version
   _phonon_find_version()
else(PHONON_FOUND)
   if(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)
      set(PHONON_FIND_QUIETLY TRUE)
   endif(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)

   find_library(PHONON_LIBRARY_RELEASE NAMES phonon phonon4 HINTS ${KDE4_LIB_INSTALL_DIR} ${QT_LIBRARY_DIR})
   find_library(PHONON_LIBRARY_DEBUG NAMES phonond phonond4 HINTS ${KDE4_LIB_INSTALL_DIR} ${QT_LIBRARY_DIR})

   # if the release- as well as the debug-version of the library have been found:
   IF (PHONON_LIBRARY_DEBUG AND PHONON_LIBRARY_RELEASE)
     # if the generator supports configuration types then set
     # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
     IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
       SET(PHONON_LIBRARY       optimized ${PHONON_LIBRARY_RELEASE} debug ${PHONON_LIBRARY_DEBUG})
     ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
       # if there are no configuration types and CMAKE_BUILD_TYPE has no value
       # then just use the release libraries
       SET(PHONON_LIBRARY       ${PHONON_LIBRARY_RELEASE} )
     ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
   ELSE(PHONON_LIBRARY_DEBUG AND PHONON_LIBRARY_RELEASE)
     IF (PHONON_LIBRARY_RELEASE)
       SET(PHONON_LIBRARY ${PHONON_LIBRARY_RELEASE})
     ENDIF (PHONON_LIBRARY_RELEASE)
   ENDIF (PHONON_LIBRARY_DEBUG AND PHONON_LIBRARY_RELEASE)

   find_path(PHONON_INCLUDE_DIR NAMES phonon/phonon_export.h HINTS ${KDE4_INCLUDE_INSTALL_DIR} ${QT_INCLUDE_DIR} ${INCLUDE_INSTALL_DIR} ${QT_LIBRARY_DIR})

   if(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)
      set(PHONON_LIBS ${phonon_LIB_DEPENDS} ${PHONON_LIBRARY})
      set(PHONON_INCLUDES ${PHONON_INCLUDE_DIR}/KDE ${PHONON_INCLUDE_DIR})
      set(PHONON_FOUND TRUE)
      _phonon_find_version()
   else(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)
      set(PHONON_FOUND FALSE)
   endif(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)

   include(FindPackageHandleStandardArgs)
   find_package_handle_standard_args(Phonon  DEFAULT_MSG  PHONON_INCLUDE_DIR PHONON_LIBRARY)

   mark_as_advanced(PHONON_INCLUDE_DIR PHONON_LIBRARY)
endif(PHONON_FOUND)
