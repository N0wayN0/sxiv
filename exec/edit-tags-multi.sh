
echo "$#"
if [ "$#" -eq 1 ]; then
    echo "only one: $1"
    tags=$(tagi.py get_tags "$1" 2>/dev/null)
    path=$(realpath "$1")

    if [ "$tags" ]; then
        tags=$(echo "$tags" | tr "\n" ",")
    fi

    user=$(echo -n "$tags" | yad --center --width=1200 --text-info --editable --wrap  -listen  \
        --fontname="Serif bold 20" --fore=black --back=white  \
        --quoted-output --class=float \
        --text="<span foreground='brown'><b><big><big>Please enter your tags:\n</big></big></b></span> \
<span foreground='green'><b><big><big>$path</big></big></b></span>")

    if [ $? -eq 0 ]
        then
            echo "tags:  $user"
            if [ "$user" ]; then
                echo "updating $1"
                tagi.py update "$path" "$user"
            else
                echo "removing $1"
                tagi.py remove "$path"
            fi
    fi

else    # many files
    path=$(dirname "$1")
    files=""
    for arg in "$@"; do
        files="$# files"
    done
    user=$(yad --center --width=1200 --text-info --editable --wrap  -listen  \
        --fontname="Serif bold 20" --fore=black --back=white  \
        --quoted-output --class=float \
        --text="<span foreground='brown'><b><big><big>Please enter tags for this $# files:\n</big></big></b></span> \
<span foreground='green'><b><big><big>$files\n$path</big></big></b></span>")

    if [ $? -eq 0 ]; then
        for file in "$@"; do
            echo "add $file $user"
            tagi.py add "$file" "$user"
        done
    fi
fi
