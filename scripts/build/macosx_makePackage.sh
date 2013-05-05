#!/bin/bash

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
QUASSEL_VERSION=$(git-describe)
BUILDTYPE=$1

# check the working dir
WORKINGDIR=$2
if [[ ! -n $2 ]]; then
    WORKINGDIR="."
fi
WORKINGDIR="${WORKINGDIR}/"
PACKAGETMPDIR="${WORKINGDIR}PACKAGE_TMP_DIR_${BUILDTYPE}"
if [[ $BUILDTYPE = "Core" ]] || [[ $BUILDTYPE = "Client" ]]; then
    QUASSEL_DMG="Quassel${BUILDTYPE}_MacOSX-x86_64_${QUASSEL_VERSION}.dmg"
    mkdir $PACKAGETMPDIR
    if [[ $BUILDTYPE = "Client" ]]; then
	cp -r ${WORKINGDIR}Quassel\ Client.app ${PACKAGETMPDIR}/
	${SCRIPTDIR}/macosx_DeployApp.py "${PACKAGETMPDIR}/Quassel Client.app"
    else
	cp ${WORKINGDIR}quasselcore ${PACKAGETMPDIR}/
	${SCRIPTDIR}/macosx_DeployApp.py --nobundle ${PACKAGETMPDIR}
    fi
    #hdiutil create -srcfolder ${PACKAGETMPDIR} -format UDBZ -volname "Quassel ${BUILDTYPE} - ${QUASSEL_VERSION}" "${WORKINGDIR}Quassel${BUILDTYPE}_MacOSX-universal_${QUASSEL_VERSION}.dmg" >/dev/null
    hdiutil create -srcfolder ${PACKAGETMPDIR} -format UDBZ -volname "Quassel ${BUILDTYPE} - ${QUASSEL_VERSION}" "${WORKINGDIR}${QUASSEL_DMG}" >/dev/null
    rm -rf ${PACKAGETMPDIR}
else
    echo >&2 "Valid parameters are \"Client\" or \"Core\""
fi
