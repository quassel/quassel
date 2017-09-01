# Copied from https://raw.github.com/facebook/hiphop-php/master/CMake/FindLdap.cmake

# - Try to find the LDAP client libraries
# Once done this will define
#
#  LDAP_FOUND - system has libldap
#  LDAP_INCLUDE_DIR - the ldap include directory
#  LDAP_LIBRARIES - libldap + liblber (if found) library
#  LBER_LIBRARIES - liblber library

if(LDAP_INCLUDE_DIR AND LDAP_LIBRARIES)
    # Already in cache, be silent
    set(Ldap_FIND_QUIETLY TRUE)
endif(LDAP_INCLUDE_DIR AND LDAP_LIBRARIES)

# Attempt to link against ldap.h regardless of platform!
FIND_PATH(LDAP_INCLUDE_DIR ldap.h)

# If we detect path to invalid ldap.h on osx, try /usr/include/
# This might also be achievable with additional parameters to FIND_PATH.
string(TOLOWER ${LDAP_INCLUDE_DIR} ldapincludelower)
if("${ldapincludelower}" MATCHES "\\/system\\/library\\/frameworks\\/ldap\\.framework\\/headers")
  set(LDAP_INCLUDE_DIR "/usr/include/")
endif()

FIND_LIBRARY(LDAP_LIBRARIES NAMES ldap)

# On osx remove invalid ldap.h
string(TOLOWER ${LDAP_LIBRARIES} ldaplower)
if("${ldaplower}" MATCHES "\\/system\\/library\\/frameworks\\/ldap\\.framework")
  set(LDAP_LIBRARIES FALSE)
endif()

FIND_LIBRARY(LBER_LIBRARIES NAMES lber)

# It'd be nice to link against winldap on Windows, unfortunately
# the interfaces are different. In theory a compatibility shim
# could be written; if someone ever gets around to doing that these
# lines should be uncommented and used on Windows.
#   FIND_PATH(LDAP_INCLUDE_DIR winldap.h)
#   FIND_LIBRARY(LDAP_LIBRARIES NAMES wldap32)

if(LDAP_INCLUDE_DIR AND LDAP_LIBRARIES)
   set(LDAP_FOUND TRUE)
   if(LBER_LIBRARIES)
     set(LDAP_LIBRARIES ${LDAP_LIBRARIES} ${LBER_LIBRARIES})
   endif(LBER_LIBRARIES)
endif(LDAP_INCLUDE_DIR AND LDAP_LIBRARIES)

if(LDAP_FOUND)
   if(NOT Ldap_FIND_QUIETLY)
      message(STATUS "Found ldap: ${LDAP_LIBRARIES}")
   endif(NOT Ldap_FIND_QUIETLY)
else(LDAP_FOUND)
   if (Ldap_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find ldap")
   endif (Ldap_FIND_REQUIRED)
endif(LDAP_FOUND)

MARK_AS_ADVANCED(LDAP_INCLUDE_DIR LDAP_LIBRARIES LBER_LIBRARIES LDAP_DIR)
