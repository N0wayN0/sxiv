#!/bin/bash

# open pcmanfm in given path and copy filename to clipboard
# then press ctrl+f ctrl+v it will highlight a file
#
target=$(realpath "$1")
location=$(dirname "$target")
filename=$(basename "$target")

cd "$location"
echo -n $filename | xclip -i -selection clipboard
pcmanfm
exit

winid=$(xdotool search --class "FMpreview" | head -n1 )
#winid=$(xdotool search --class "FMpreview")
echo $winid
if [[ ! $winid ]];then
    pcmanfm --class "FMpreview" &
    sleep 0.2
    winid=$(xdotool search --class "FMpreview" | head -n1 )
fi

xdotool key --window $winid ctrl+l
xdotool type --window $winid $location
xdotool key --window $winid ctrl+a
xdotool key --window $winid Return

echo -n $filename | xclip -i -selection clipboard
#xdotool key --window $winid ctrl+f
#xdotool type --window $winid $filename

