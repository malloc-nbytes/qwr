#!/bin/bash

set -xe

files=$(find . -type f -name '*.c')
libs=$(forge lib)
name="qwr"
cc="cc"

$cc -o $name $files $libs
