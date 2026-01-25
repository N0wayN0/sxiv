#!/bin/sh

while read -r file
do
    case "$1" in
        "s") tag="sex" ;;
        "c") tag="cipka" ;;
        "b") tag="bj" ;;
    esac

if [ "$tag" ];then
    notify-send "$file tag as $tag" &
    eval tagi.py add "$file" "$tag"
fi
done
