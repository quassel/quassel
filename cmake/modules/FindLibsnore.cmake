########################################################################################
# Copyright (c) 2010 Patrick von Reth <patrick.vonreth@gmail.com>                      #
#                                                                                      #
# This program is free software; you can redistribute it and/or modify it under        #
# the terms of the GNU General Public License as published by the Free Software        #
# Foundation; either version 2 of the License, or (at your option) any later           #
# version.                                                                             #
#                                                                                      #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY      #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.             #
#                                                                                      #
# You should have received a copy of the GNU General Public License along with         #
# this program.  If not, see <http://www.gnu.org/licenses/>.                           #
########################################################################################

# - Try to find the libsnore library
# Once done this will define
#
#  LIBSNORE_FOUND - system has the LIBSNORE library
#  LIBSNORE_LIBRARIES - The libraries needed to use LIBSNORE
#  LIBSNORE_PLUGIN_PATH - Path of the plugins


find_library(LIBSNORE_LIBRARIES NAMES libsnore snore)
find_path(LIBSNORE_PLUGIN_PATH snoreplugins)
if(LIBSNORE_LIBRARIES AND LIBSNORE_PLUGIN_PATH)
    set(LIBSNORE_FOUND TRUE)
    set(LIBSNORE_PLUGIN_PATH ${LIBSNORE_PLUGIN_PATH}/snoreplugins)
    message(STATUS "Found libsnore ${LIBSNORE_LIBRARIES}")
else(LIBSNORE_LIBRARIES AND LIBSNORE_PLUGIN_PATH)
    message(STATUS "Could not find libsnore, please install libsnore")
endif(LIBSNORE_LIBRARIES AND LIBSNORE_PLUGIN_PATH)
