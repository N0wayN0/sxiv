#!/bin/bash

# this script open YAD and remove file passed as parameter

yad --center --width=600 height=400 --text=" Are you sure you want to remove this file?\n\n$1" --button=gtk-ok:0 --button=gtk-no:1

if [ $? -eq 0 ] && [ "$1" ] 
then
    rm "$1"
	if [ -f "$1" ]  # if file still exists
    then
        message="Filed to removed file"
        yad --center --timeout=1 --no-buttons --text "$message" 
    else
        message="File removed succesfully"  # not needed
    fi
fi
