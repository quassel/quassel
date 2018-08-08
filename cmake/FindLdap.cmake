#.rst:
# FindLdap
# --------
#
# Try to find the LDAP client libraries.
#
# This will define the following variables:
#
# ``Ldap_FOUND``
#     True if libldap is available.
#
# ``Ldap_VERSION``
#     The version of libldap
#
# ``Ldap_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if
#     the target is not used for linking
#
# ``Ldap_LIBRARIES``
#     The LDAP libraries (libldap + liblber if available)
#     This can be passed to target_link_libraries() instead of
#     the ``Ldap::Ldap`` target
#
# If ``Ldap_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``Ldap::Ldap``
#     The LDAP library
#
#=============================================================================
# Copyright 2006 Szombathelyi György <gyurco@freemail.hu>
# Copyright 2007-2016 Laurent Montel <montel@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

if (NOT APPLE)
    find_path(Ldap_INCLUDE_DIRS NAMES ldap.h)
    find_library(Ldap_LIBRARY NAMES ldap)
    find_library(Lber_LIBRARY NAMES lber)
else()
    # OSX ships with an LDAP.framework which seems to be completely broken.
    # Force using the working version in /usr instead.
    find_path(Ldap_INCLUDE_DIRS NAMES ldap.h PATHS /usr/include NO_CMAKE_SYSTEM_PATH)
    find_library(Ldap_LIBRARY NAMES ldap PATHS /usr/lib NO_CMAKE_SYSTEM_PATH)
    find_library(Lber_LIBRARY NAMES lber PATHS /usr/lib NO_CMAKE_SYSTEM_PATH)
endif()

set(Ldap_LIBRARIES ${Ldap_LIBRARY})
if (Ldap_LIBRARY AND Lber_LIBRARY)
    list(APPEND Ldap_LIBRARIES ${Lber_LIBRARY})
endif()

if(EXISTS ${Ldap_INCLUDE_DIRS}/ldap_features.h)
    file(READ ${Ldap_INCLUDE_DIRS}/ldap_features.h LDAP_FEATURES_H_CONTENT)
    string(REGEX MATCH "#define LDAP_VENDOR_VERSION_MAJOR[ ]+[0-9]+" _LDAP_VERSION_MAJOR_MATCH ${LDAP_FEATURES_H_CONTENT})
    string(REGEX MATCH "#define LDAP_VENDOR_VERSION_MINOR[ ]+[0-9]+" _LDAP_VERSION_MINOR_MATCH ${LDAP_FEATURES_H_CONTENT})
    string(REGEX MATCH "#define LDAP_VENDOR_VERSION_PATCH[ ]+[0-9]+" _LDAP_VERSION_PATCH_MATCH ${LDAP_FEATURES_H_CONTENT})

    string(REGEX REPLACE ".*_MAJOR[ ]+(.*)" "\\1" LDAP_VERSION_MAJOR ${_LDAP_VERSION_MAJOR_MATCH})
    string(REGEX REPLACE ".*_MINOR[ ]+(.*)" "\\1" LDAP_VERSION_MINOR ${_LDAP_VERSION_MINOR_MATCH})
    string(REGEX REPLACE ".*_PATCH[ ]+(.*)" "\\1" LDAP_VERSION_PATCH ${_LDAP_VERSION_PATCH_MATCH})

    set(Ldap_VERSION "${LDAP_VERSION_MAJOR}.${LDAP_VERSION_MINOR}.${LDAP_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ldap
    FOUND_VAR Ldap_FOUND
    REQUIRED_VARS Ldap_LIBRARIES Ldap_INCLUDE_DIRS
    VERSION_VAR Ldap_VERSION
)

if (Ldap_FOUND AND NOT TARGET Ldap::Ldap)
    add_library(Ldap::Ldap UNKNOWN IMPORTED)
    set_target_properties(Ldap::Ldap PROPERTIES
        IMPORTED_LOCATION "${Ldap_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${Lber_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Ldap_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Ldap_INCLUDE_DIRS Ldap_LIBRARY Lber_LIBRARY Ldap_LIBRARIES Ldap_VERSION)
