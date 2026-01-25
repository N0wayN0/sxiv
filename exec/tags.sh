
#echo start; read -n1

echo "$#"
if [ "$#" -eq 1 ]; then
    echo "only one: $1"
    tags="jedwn,dwa,trzy"
    #tags=$(tagi.py get_tags "$1" 2>/dev/null)
    echo " orginaal $tags"
    path=$(realpath "$1")

    if [ "$tags" ]; then
        tags=$(echo "$tags" | tr "\n" ",")
    fi

    mkfifo rurka
    #echo "$tags" > rurka &
    trap "rm -f rurka" EXIT
    exec 3<> rurka
    echo -e "\f" > rurka    # to czysci wszystko

#echo run vim; read -n1
    st -n "floating" -g 120x10+200+300 -e bash -c "echo $tags | vim -u ~/.vim/taging.rc -"
    #user=$(st -n "floating" -g 120x10+200+300 -e bash -c "cat rurka | vim -u ~/.vim/taging.rc - ;read -n1")
    
    #cat rurka
    cat <&3 &
    #user=$(cat rurka )
    echo $user
#exec 3>&-   # close pipe i przywruc stabdard I/O czyli screen -
#echo koniec; read -n1
exit

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
    user=$()

    if [ $? -eq 0 ]; then
        for file in "$@"; do
            echo "add $file $user"
            tagi.py add "$file" "$user"
        done
    fi
fi
