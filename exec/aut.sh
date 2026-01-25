#!/bin/bash

#echo -e "\f" > rurka    # to czysci wszystko
while read -r line; do
    echo "Processed: $line"
    notify-send "$line"
    echo "$line" > rurka &
done
