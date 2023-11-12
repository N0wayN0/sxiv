#!/bin/bash

st -n "floating" -g 120x40+200+60 -e bash -c "mediainfo $1; echo TAGS:; tagi.py get_tags $1 2>/dev/null;  read -n1"
