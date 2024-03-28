#!/bin/bash

# this script open YAD and remove file passed as parameter

#yad --class=float --center --width=600 --height=200 --text=" Are you sure you want to remove this file(s)?\n\n$@" --button=gtk-ok:0 --button=gtk-no:1


args=("$@")

printf "%s\n" "${args[@]}" | yad --class=float --list --center --width=1200  --wrap --listen  \
    --fontname="Serif bold 20" --fore=black --back=white  \
    --quoted-output --button=gtk-ok:0 --button=gtk-no:1 \
    --text="<span foreground='brown'><b><big><big>\nAre you sure you want to remove this $# file(s):</big></big></b></span>" \
    --column="File path"  \
    --search-column=1 --expand-column=0 

if [ $? -eq 0 ]
then
    for file in "$@"; do
        rm "$file"
	    if [ -f "$file" ]  # if file still exists
            then
                message="Filed to removed file\n$file"
                yad --center --timeout=1 --no-buttons --text "$message" 
        else
                message="File removed succesfully"  # not needed
        fi
    done
fi
