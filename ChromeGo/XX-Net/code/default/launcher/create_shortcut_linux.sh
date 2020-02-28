#!/bin/bash

# variable XXNETPATH is env by python call

function createDesktopStartup() {
    DESKDIR=~/Desktop/

    if [[ -d $DESKDIR ]]; then
        DESKFILE="$DESKDIR/XX-Net.desktop"
    else
        echo "$DESKDIR does not exist"
        return -1
    fi

    # python won't call this script if lastrun in config matchs the current run

    #if [[ -f $DESKFILE ]]; then
    #    echo "$DESKFILE already exists"
    #    return
    #else
    #    echo "$DESKFILE does not exist,create a new one"
    #fi

    NAME="XX-Net"
    EXEC="$XXNETPATH/start > /dev/null"
    ICON="$XXNETPATH/code/default/launcher/web_ui/favicon.ico"
    TERMINAL="false"
    TYPE="Application"
    CATEGORIES="Development"
    echo "[Desktop Entry]" > "$DESKFILE"
    #echo "Version=$VERSION" >> "$DESKFILE"
    echo Name=$NAME >> "$DESKFILE"
    echo 'Exec="$EXEC"' >> "$DESKFILE"
    echo Terminal=$TERMINAL >> "$DESKFILE"
    echo 'Icon="$ICON"' >> "$DESKFILE"
    echo Type=$TYPE >> "$DESKFILE"
    echo Categories=$CATEGORIES >> "$DESKFILE"

    chmod 744 $DESKFILE

}

# create a desktop startup file when the distro is Ubuntu.
DIS=`cat /etc/issue 2> /dev/null`
if [[ $DIS == *Ubuntu* || $DIS == *Debian* ]]; then
    createDesktopStartup
fi
