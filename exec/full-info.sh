#!/bin/bash

function show_tags() {
    while true;do
        clear
        echo "c - cipka"
        echo "d - dupa"
        echo "f - feet"
        echo "s - sex"
        echo "b - bj"
        echo "v - fpv"
        echo "m - milf"
        echo "w - wife"
        echo "h - heels"
        echo "o - obspermiona"
        echo "t - on top"

        read -n1 choice > /dev/null
        case "$choice" in
            c) tag="cipka" ;;
            d) tag="dupa" ;;
            f) tag="feet" ;;
            s) tag="sex" ;;
            b) tag="bj" ;;
            v) tag="fpv" ;;
            m) tag="milf" ;;
            w) tag="wife" ;;
            h) tag="heels" ;;
            t) tag="on top" ;;
            o) tag="obspermiona" ;;
            q) exit ;;
        esac
        if [[ -z "$tag" ]];then
            echo pusty
        else
            echo "add $1 \"$tag\""
            tagi.py add "$1" "$tag" 
            tag=""
        fi
    done
}

export -f show_tags
#st -n "floating" -g 120x32+200+60 -e bash -c "show_tags $1"
#exit

function show_info() {
    all_info=$(tagi.py get_info "$1" 2>/dev/null)
    tags=$(printf '%s\n' "$all_info" | grep "^tags")
    tags=$(echo "${tags:6}")
    file_in_DB=$(printf '%s\n' "$all_info" | grep "^file")
    file_in_DB=$(echo "${file_in_DB:6}")
    mediainfo "$1"
    echo "Records from database:"
    echo TAGS: $tags
    echo $file_in_DB

    echo --- duplicate files ----
    find_same.sh "$1" /home/
    echo --- END ----
    echo

    if [[ "$file_in_DB" == "$1" ]];then
        echo "Database record points to THIS file"
    elif [[ "$file_in_DB" ]];then
        echo "Database record does mot match"
        echo DB $file_in_DB
        if [[ ! -f "$file_in_DB" ]];then
            echo "file in BD does not exists"
            echo "[u] update this file in database"
            echo "[U] update all files in this directory"
            read -n1 choice
            case "$choice" in
                u) update_path "$1" ;;
                U) update_path $(dirname "$1") ;;
                *) exit ;;
            esac
        else
            echo "file from DB exists"
        fi
    fi
    read -n1
}
export -f show_info

st -n "floating" -g 120x32+200+60 -e bash -c "show_info $1"
