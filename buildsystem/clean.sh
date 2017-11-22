#!/bin/sh
find .. -name moc_* -type f -delete
rm -rf ../build
rm -f ../gui/qresource.cpp
