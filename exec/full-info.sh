#!/bin/bash

#st -e fzf-move.sh "$1"
st -n "floating" -g 120x40+200+60 -e bash -c "mediainfo $1; read -n1"
