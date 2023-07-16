#!/bin/sh

cd ../flipperzero-firmware
test -d applications_user/logical-anal || ln -s ../../logical-anal applications_user/

./fbt launch_app APPSRC=logical-anal
