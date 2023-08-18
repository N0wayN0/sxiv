#!/bin/sh

#st -g 40x10-700+220 -e vim
tags=$(tagi.py get_tags "$1" 2>/dev/null)
path=$(realpath "$1")

if [ "$tags" ]
    then
        tags=$(echo "$tags" | tr "\n" ",")
fi

user=$(echo -n "$tags" | yad --center --width=1200 --text-info --editable --wrap  -listen  \
    --fontname="Serif bold 20" --fore=black --back=white  \
    --quoted-output \
    --text="<span foreground='brown'><b><big><big>Please enter your tags:\n</big></big></b></span> \
<span foreground='green'><b><big><big>$path</big></big></b></span>")

if [ $? -eq 0 ]
    then
        #tagi=`echo $user | tr "|" " "`
        #tagi.py add "$path" "$tagi"
        tagi.py update "$path" "$user"
fi

