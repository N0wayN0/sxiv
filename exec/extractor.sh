#!/bin/sh


while read -r file
do
    notify-send "$file moved to $1" &
    mkdir -p "$1"
    mv "$file" "$1"

done
