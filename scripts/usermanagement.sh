#!/bin/sh
#
# Delete Quasselcore users from your SQLite database
#

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


