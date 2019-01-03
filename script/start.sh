#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR/..
#kill Old
./script/killAll.sh

#Start python both of
node openstagecontrol/bin/open-stage-control -n -l openstagecontrol/anima.json -s 127.0.0.1:12345 -o 11001  &
python python/midi.py & 
./of/bin/of 



