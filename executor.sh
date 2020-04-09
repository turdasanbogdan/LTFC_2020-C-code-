#!/bin/bash

gcc -c -o exec alex.c
gcc -o exec main.h
gcc -c -o exec main.c 
chmod +x exec 
./exec "file.txt"

