#!/bin/bash 
if [[ "$(uname)" == "Darwin" ]]; then
    tundra2 "$@"
else
    ./Tools/tundra-linux-2.0/bin/tundra2 "$@"
fi
