#!/bin/sh
# probe data post process start on boot
# Mon Nov  2 10:00:54 CST 2015 created by TianyuanPan
#

OPTIONS=" -d 4 -s"

/usr/bin/wifiSmartAc $OPTIONS &> /dev/null

