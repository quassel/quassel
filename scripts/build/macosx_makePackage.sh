#!/bin/bash
# Don't consider packaging a success if any commands fail
# See http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

myname=$0
if [ -s "$myname" ] && [ -x "$myname" ]; then
    # $myname is already a valid file name

    mypath=$myname
else
    case "$myname" in
    /*) exit 1;;             # absolute path - do not search PATH
    *)
        # Search all directories from the PATH variable. Take
        # care to interpret leading and trailing ":" as meaning
        # the current directory; the same is true for "::" within
        # the PATH.

        # Replace leading : with . in PATH, store in p
        p=${PATH/#:/.:}
        # Replace trailing : with .
        p=${p//%:/:.}
        # Replace :: with :.:
        p=${p//::/:.:}
        # Temporary input field separator, see FAQ #1
        OFS=$IFS IFS=:
        # Split the path on colons and loop through each of them
        for dir in $p; do
                [ -f "$dir/$myname" ] || continue # no file
                [ -x "$dir/$myname" ] || continue # not executable
                mypath=$dir/$myname
                break           # only return first matching file
        done
        # Restore old input field separator
        IFS=$OFS
        ;;
    esac
fi

if [ ! -f "$mypath" ]; then
    echo >&2 "cannot find full path name: $myname"
    exit 1
fi

SCRIPTDIR=$(dirname $mypath)
QUASSEL_VERSION=$(git describe)
BUILDTYPE=$1

# check the working dir
# Default to "." using Bash default-value syntax
WORKINGDIR="${2:-.}"
WORKINGDIR="${WORKINGDIR}/"
PACKAGETMPDIR="${WORKINGDIR}PACKAGE_TMP_DIR_${BUILDTYPE}"
QUASSEL_DMG="Quassel${BUILDTYPE}_MacOSX-x86_64_${QUASSEL_VERSION}.dmg"

# Default to null string
if [[ -z ${3:-} ]]; then
	ADDITIONAL_PLUGINS=""
else
	# Options provided, append to list
	ADDITIONAL_PLUGINS=",$3"
fi

echo "ADDITIONAL_PLUGINS: ${ADDITIONAL_PLUGINS}"

mkdir $PACKAGETMPDIR
case $BUILDTYPE in
"Client")
	cp -r ${WORKINGDIR}Quassel\ Client.app ${PACKAGETMPDIR}/
	${SCRIPTDIR}/macosx_DeployApp.py --plugins=qcocoa,qgenericbearer,qcorewlanbearer,qmacstyle${ADDITIONAL_PLUGINS} "${PACKAGETMPDIR}/Quassel Client.app"
	;;
"Core")
	cp ${WORKINGDIR}quasselcore ${PACKAGETMPDIR}/
	${SCRIPTDIR}/macosx_DeployApp.py --nobundle --plugins=qsqlite,qsqlpsql${ADDITIONAL_PLUGINS} ${PACKAGETMPDIR}
	;;
"Mono")
	cp -r ${WORKINGDIR}Quassel.app ${PACKAGETMPDIR}/
	${SCRIPTDIR}/macosx_DeployApp.py --plugins=qsqlite,qsqlpsql,qcocoa,qgenericbearer,qcorewlanbearer,qmacstyle${ADDITIONAL_PLUGINS} "${PACKAGETMPDIR}/Quassel.app"
	;;
*)
	echo >&2 "Valid parameters are \"Client\", \"Core\", or \"Mono\"."
	rmdir ${PACKAGETMPDIR}
	exit 1
	;;
esac

echo "Creating macOS disk image with hdiutil: 'Quassel ${BUILDTYPE} - ${QUASSEL_VERSION}'"

# Modern macOS versions support APFS, however default to HFS+ for now in order
# to ensure old macOS versions can parse the package and display the warning
# about being out of date.  This mirrors the approach taken by Qt's macdeployqt
# tool.  In the future if this isn't needed, just remove "-fs HFS+" to revert
# to default.
#
# See https://doc.qt.io/qt-5/macos-deployment.html

# hdiutil seems to have a bit of a reputation for failing to create disk images
# for various reasons.
#
# If you've come here to see why on earth your macOS build is failing despite
# making changes entirely unrelated to macOS, you have my sympathy.
#
# There are two main approaches:
#
# 1.  Let hdiutil calculate a size automatically
#
# 2.  Separately calculate the size with a margin of error, then specify this
#     to hdiutil during disk image creation.
#
# Both seem to have caused issues, but in recent tests, option #1 seemed more
# reliable.
#
# Option 1:

hdiutil create -srcfolder ${PACKAGETMPDIR} -format UDBZ -fs HFS+ -volname "Quassel ${BUILDTYPE} - ${QUASSEL_VERSION}" "${WORKINGDIR}${QUASSEL_DMG}" >/dev/null

# If hdiutil changes over time and fails often, you can try the other option.
#
# Option 2:
#
#PACKAGESIZE_MARGIN="1.1"
#PACKAGESIZE=$(echo "$(du -ms ${PACKAGETMPDIR} | cut -f1) * $PACKAGESIZE_MARGIN" | bc)
#echo "PACKAGESIZE: $PACKAGESIZE MB"
#hdiutil create -srcfolder ${PACKAGETMPDIR} -format UDBZ -fs HFS+ -size ${PACKAGESIZE}M -volname "Quassel ${BUILDTYPE} - ${QUASSEL_VERSION}" "${WORKINGDIR}${QUASSEL_DMG}" >/dev/null


# Regardless of choice, clean up the packaging temporary directory
rm -rf ${PACKAGETMPDIR}
