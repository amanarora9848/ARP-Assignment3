#! /usr/bin/bash

gcc -fdiagnostics-color=always -g src/processA.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processA
gcc -fdiagnostics-color=always -g src/processB.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processB
gcc -fdiagnostics-color=always -g src/master.c -lpthread -o bin/master

echo "Compiled."