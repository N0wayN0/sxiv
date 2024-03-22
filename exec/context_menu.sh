#!/bin/bash

export dir="$(dirname $1)"

xmenu <<EOF | sh &
"$1"
"$dir"
PCManFM "$1"	pcmanfm "$dir"
sxiv in this folder     "$dir"
RotatrRight		convert -rotate 90 "$1" "$1"
Find duplicates	find_dupes.yad.sh
IMG:/usr/share/icons/hicolor/32x32/apps/mpv.png	mpv Media Player	mpv --player-operation-mode=pseudo-gui -- "$1"
IMG:/usr/share/icons/Adwaita/48x48/emblems/emblem-photos.png	 Wallpaper	setbg   "$1"
IMG:/root/.config/xmenu/window-close.png	 Close Window	xdotool selectwindow windowclose

IMG:/root/.config/xmenu/terminal.png	 Terminal (st)	st
EOF
