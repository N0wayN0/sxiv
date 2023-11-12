#!/bin/bash

dirname=$(dirname "$1")
cd  "$dirname"
st -n "floating" -g 120x40+200+60 -e "/bin/bash"

