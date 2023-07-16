#!/bin/sh

cd ../flipperzero-firmware
test -d applications_user/logical-anal || ln -s ../../logical-anal applications-user/

./fbt fap_logical_anal
