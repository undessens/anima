#!/bin/bash
exec 1>/var/log/anima.log 2>&1  # send stdout and stderr from rc.local to a log file
set -x                         # tell sh to display commands before execution

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR/..
#kill Old
echo "killing Old Processes"
./script/killAll.sh

#Start python both of
echo "starting New Processes"
./script/runOSC.sh > /var/log/openstagecontrol.log  2>&1 &
python python/midi.py > /var/log/python_anima.log 2>&1 & 
./of/bin/of > /var/log/of_anima.log  2>&1 & 

echo "getting out of script"



