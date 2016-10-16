#!/bin/sh
COMMAND=$1
    
THEKEXT=/System/Library/Extensions/HelloWorld.kext

if [ -f "$THEKEXT" ]
then
    echo "KEXT does not exist"
    exit 1
fi
if [ "$COMMAND" = "load" ]
then
        kextload $THEKEXT
elif [ "$COMMAND" = "unload" ]
then
        kextunload $THEKEXT
fi
