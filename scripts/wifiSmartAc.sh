#!/bin/sh
# probe data post process start on boot
# Mon Nov  2 10:00:54 CST 2015 created by GaomingPan
#

OPTIONS=" -d 4 -s"

/usr/sbin/WifiSmartAc $OPTIONS > /dev/null & 2> /dev/null

