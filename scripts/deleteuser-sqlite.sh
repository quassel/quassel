#!/bin/sh
#
# Delete Quasselcore users from your SQLite database
#
# File: deleteuser-sqlite.sh
# Author: Robbe Van der Gucht
# License: BSD-3-Clause, GPLv2, GPLv3
# License statements can be found at the bottom.
# Any of the indicated licenses can be chosen for redistribution 
# and only requires one of the license statements to be preserved.

exeq()
{
    # Execute SQL Query
    result=$(sqlite3 "${QUASSELDB}" "${1}")
    echo "${result}"
}

usage()
{
    echo "Usage: ${SCRIPT} username [database]"
}

print_users()
{
    sqlite3 "${QUASSELDB}" "SELECT quasseluser.userid, quasseluser.username FROM quasseluser ORDER BY quasseluser.userid;"
}

# Main body

SCRIPT="${0}"
QUASSELDB=""
USER=""

if [ -z "${2}" ] ; then
    # No file supplied.
    QUASSELDB="quassel-storage.sqlite"
else
    QUASSELDB="${2}"
fi

if [ -z "${1}" ] ; then
    echo "No user supplied."
    echo "Pick one: "
    print_users
    usage
    exit 1
else
    USER="${1}"
fi

if [ -e "${QUASSELDB}" ] ; then
    echo "SELECTED DB: ${QUASSELDB}"
else
    echo "SELECTED DB '${QUASSELDB}' does not exist."
    usage
    exit 2
fi

if [ -z $(exeq "SELECT quasseluser.username FROM quasseluser WHERE username = '${USER}';") ] ; then
    echo "SELECTED USER '${USER}' does not exist."
    print_users
    usage
    exit 3
else
    echo "SELECTED USER: ${USER}"
fi

# Sadly SQLITE does not allow DELETE statements that JOIN tables.
# All queries are written with a subquery.
# Contact me if you know a better way.

backlogq="DELETE
FROM backlog
WHERE backlog.bufferid in (
    SELECT bufferid
    FROM buffer, quasseluser
    WHERE buffer.userid = quasseluser.userid
    AND quasseluser.username = '${USER}'
);"

bufferq="DELETE
FROM buffer
WHERE buffer.userid in (
    SELECT userid
    FROM quasseluser
    WHERE quasseluser.username = '${USER}'
);"

ircserverq="DELETE
FROM ircserver
WHERE ircserver.userid in (
    SELECT userid
    FROM quasseluser
    WHERE quasseluser.username = '${USER}'
);"

identity_nickq="DELETE
FROM identity_nick
WHERE identity_nick.identityid in (
    SELECT identityid
    FROM quasseluser, identity
    WHERE quasseluser.userid = identity.userid
    AND quasseluser.username = '${USER}'
);"

identityq="DELETE
FROM identity
WHERE identity.userid in (
    SELECT userid
    FROM quasseluser
    WHERE quasseluser.username = '${USER}'
);"

networkq="DELETE
FROM network
WHERE network.userid in (
    SELECT userid
    FROM quasseluser
    WHERE quasseluser.username = '${USER}'
);"

usersettingq="DELETE
FROM user_setting
WHERE user_setting.userid in (
    SELECT userid
    FROM quasseluser
    WHERE quasseluser.username = '${USER}'
);"

quasseluserq="DELETE
FROM quasseluser
WHERE quasseluser.username = '${USER}'
;"


exeq "${backlogq}"
exeq "${bufferq}"
exeq "${ircserverq}"
exeq "${identity_nickq}"
exeq "${identityq}"
exeq "${networkq}"
exeq "${usersettingq}"
exeq "${quasseluserq}"


#-----------------------------------------------------------------------------#
# BSD-3-Clause 
# Copyright (c) 2018, Robbe Van der Gucht
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#    This product includes software developed by the <organization>.
# 4. Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#-----------------------------------------------------------------------------#
# GPL Clauses
# Copyright (c) 2018, Robbe Van der Gucht
# All rights reserved.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
