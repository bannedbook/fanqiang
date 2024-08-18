#!/bin/bash

chmod -R 777 .build 2>/dev/null
rm -rf .build 2>/dev/null

# Install gomobile
if [ ! -f "$GOPATH/bin/gomobile" ]; then
    git clone https://github.com/MatsuriDayo/gomobile.git
    pushd gomobile/cmd
    pushd gomobile
    go install -v
    popd
    pushd gobind
    go install -v
    popd
    popd
    rm -rf gomobile
fi

gomobile init
