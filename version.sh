#!/bin/sh
grep "ProgramVersion::*VersionNumber" src/version.cpp \
        | awk '{print $3;}' \
        | sed s/[\(\"\;\)]//g
