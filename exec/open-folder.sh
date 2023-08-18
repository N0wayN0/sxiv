#!/bin/bash

dirname=$(dirname "$1")
cd  "$dirname"

if [ -f index.idx ]
    then
        cat index.idx | sxiv -b -f -s f -
        exit
else	
sxiv -b -f -s f "$1" .
fi

