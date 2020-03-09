#!/bin/bash

gcc -o exec alex.c
gcc -o exec main.h
gcc -o exec main.c 
./exec "file.txt"

