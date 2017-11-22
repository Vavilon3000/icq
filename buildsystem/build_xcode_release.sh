#!/bin/sh
cd ../build
if [ $? -eq 0 ]; then
    /usr/bin/xcodebuild -target ICQ -configuration Release
else
    echo Unable to change directory
fi

