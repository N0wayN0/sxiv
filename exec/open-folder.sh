#!/bin/bash

dest=$(dirname "$1")
cd  "$dest"

export SXIV_TITLE=$dest
export SXIV_TARGET=$1

if [ -f index.idx ]
    then
        cat index.idx | sxiv -b -f -s f -
        exit
else	
#sxiv -b -f -s f "$1" .
sxiv -b -f -s f  .
fi

