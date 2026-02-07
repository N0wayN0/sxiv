#!/bin/sh

while read -r file
do
    case "$1" in
        "2") tag="dwie" ;;
        "3") tag="threesome" ;;
        "b") tag="bj" ;;
        "d") tag="dupa" ;;
        "c") tag="cipka" ;;
        "f") tag="feet" ;;
        "g") tag="glam" ;;
        "h") tag="heels" ;;
        "l") tag="les" ;;
        "m") tag="murzynka" ;;
        "o") tag="obspermiona" ;;
        "p") tag="przed" ;;
        "r") tag="ruda" ;;
        "s") tag="sex" ;;
        "u") tag="ubrana" ;;
    esac

if [ "$tag" ];then
    notify-send "$file tag as $tag" &
    eval tagi.py add "$file" "$tag"
    #echo tagi.py add "$file" "$tag"
fi
done
