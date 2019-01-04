#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR/..


echo `node -v`
echo `which node`

OSC_SESSION=$DIR/../openstagecontrol/anima.json
echo $OSC_SESSION 
HOME=/home/pi node openstagecontrol/bin/open-stage-control -n -l $OSC_SESSION -s 127.0.0.1:12345 -o 11001 

