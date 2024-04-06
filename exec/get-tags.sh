#!/bin/sh

# podiera tagi z podanego pliku
# urzywany prze sxiv na razie

tagi.py get_tags "$1" 2>/dev/null
