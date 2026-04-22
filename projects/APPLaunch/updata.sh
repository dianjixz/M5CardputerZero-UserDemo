#!/bin/bash
ssh pi@192.168.28.153 'echo pi |  sudo -S pkill -f M5CardputerZero-UserDemo; sleep 1' 
scons push 
ssh pi@192.168.28.153 'cd /home/pi ; nohup ./dist/M5CardputerZero-UserDemo > /dev/null 2>&1 &'