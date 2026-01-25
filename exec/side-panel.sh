#!/bin/bash

tags=$(tagi.py get_tags "$1" 2>/dev/null)
path=$(realpath "$1")

if [ "$tags" ]; then
    tags=$(echo "$tags" | tr "," "\n")
fi

echo -n "$tags" | yad --posx=1 --posy=10 --width=350 --height=900 \
   --text-info --close-on-unfocus --no-buttons --wrap  -listen  \
   --fontname="Serif bold 20" --fore=black --back=white  \
   --quoted-output --class=float \
   --text="<span foreground='green'><b><big><big>$path</big></big></b></span>"

