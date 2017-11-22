#!/bin/sh
cd ../build
if [ $? -eq 0 ]; then
    /usr/bin/xcodebuild -target ICQ -configuration Debug
else
    echo Unable to change directory
fi

