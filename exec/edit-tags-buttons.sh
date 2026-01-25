#!/bin/bash

#export find_cmd='@bash -c "run_find %1 %2 %3 %4 %5"'
#export find_cmd='@bash -c "add_row %1 %2 %3 %4 %5"'

mkfifo pipe_tags
trap "rm -f pipe_*" EXIT
exec 3<> pipe_tags

function clear_all {
    echo -e "\f" > pipe_tags    # to czysci wszystko
}

function add_tag {
    echo "$@" > pipe_tags
}
export -f clear_all
export -f  add_tag

function show_panel {
    KEY=$RANDOM 
    TAGS=("sex" "alone" "les" "bj" "milf" "doggy" "on top")
    BUTTONS=""
    
    for i in "${!TAGS[@]}"; do
        BUTTONS+=" --field=\"${TAGS[$i]}\":fbtn 'bash -c \"add_tag ${TAGS[$i]}\"'"
    done
    BUTTONS+=" --field="Clear":fbtn 'bash -c "clear_all"' >/dev/null & "

    eval "yad --plug=$KEY --tabnum=1 --form --columns=5 -listen \
        --text=\"<span foreground='Black'><b><big><big>Choose tags</big></big></b></span>\" " \
$BUTTONS &
            
    yad --plug=$KEY --tabnum=2 --text-info --editable --wrap --listen  \
        --fontname="Serif bold 20" --fore=black --back=white  \
        --quoted-output --class=float --focus-field=0 \
        --text="$text" <&3 &
    
    yad --posx=300 --posy=-1 --paned --key=$KEY --width=1200 --height=800 \
        --title="Add tags" --button="gtk-ok" --button="gtk-cancel"
}


echo "$#"
if [ "$#" -eq 1 ]; then
    echo "only one: $1"
    tags=$(tagi.py get_tags "$1" 2>/dev/null)
    path=$(realpath "$1")

    if [ "$tags" ]; then
        echo "existing tags: $tags"
        #tags=$(echo "$tags" | tr "\n" ",")
        echo "$tags" > pipe_tags
    fi
    
    text="<span foreground='brown'><b><big><big>Please enter your tags:\n</big></big></b></span> \
        <span foreground='green'><b><big><big>$path</big></big></b></span>"
    user=$(show_panel)
    
    if [ $? -eq 0 ]; then
        if [[ "$user" != "$tags" ]];then
            echo "tags are different"
            if [ "$user" ]; then
                user=$(echo "$user" | tr "\n" ",")
                echo "new tags:  $user"
                echo "updating $1"
                tagi.py update "$path" "$user"
            else
                echo "removing $1"
                tagi.py remove "$path"
            fi
        fi
    fi

else    # many files
    path=$(dirname "$1")
    files=""
    for arg in "$@"; do
        files="$# files"
    done
    
    text="<span foreground='brown'><b><big><big>Please enter tags for this $# files:\n</big></big></b></span> \
    <span foreground='green'><b><big><big>$files\n$path</big></big></b></span>"
    user=$(show_panel)

    if [ $? -eq 0 ]; then
        #user=$(echo "$user" | tr "\n" ",")
        #echo "more tags:  $user"
        if [ "$user" ]; then
            for file in "$@"; do
                echo "add $file $user"
                tagi.py add "$file" "$user"
            done
        fi
    fi
fi

exec 3>&-   # close pipe i przywruc stabdard I/O czyli screen -
