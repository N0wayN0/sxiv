#!/bin/bash

dirname=$(dirname "$1")
cd  "$dirname"

st -e "/bin/bash" #&& cd -

