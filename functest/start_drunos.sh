#!/usr/bin/env bash


for k in $(seq 24 40)
do
    while true; do
        ./running_test.py drunos linear $k $k
        if [[ $? -ne 0 ]]; then
            echo "Contunue, repeat? r/c/n"
            read NEXT
            if [[ $NEXT == 'r' ]]; then
                continue
            fi
            if [[ $NEXT == 'n' ]]; then
                exit 1
            fi
        fi
        break
    done
done

