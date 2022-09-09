#!/bin/bash

MJ_VER=`grep WD_MAJOR_VER $WDSRC/include/wd_ver.h | head -n1 | cut -d " " -f3 \
    | tr -d "\r"`
MINOR_VER=`grep WD_MINOR_VER $WDSRC/include/wd_ver.h | head -n1 \
    | cut -d " " -f3 | tr -d "\r"`
SUB_MINOR_VER=`grep WD_SUB_MINOR_VER $WDSRC/include/wd_ver.h | head -n1 \
    | cut -d " " -f3 | tr -d "\r"`
WD_YEAR=`grep COPYRIGHTS_YEAR_STR $WDSRC/include/wd_ver.h | head -n1 \
    | cut -d " " -f3 | tr -d "\r"`

WD_VERSION=$MJ_VER$MINOR_VER$SUB_MINOR_VER
WD_VERSION_DOTS=$MJ_VER.$MINOR_VER.$SUB_MINOR_VER