#!/bin/bash
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 1 -f 3 -S 8 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 2 -f 3 -S 7 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 3 -f 3 -S 6 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 4 -f 3 -S 5 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 5 -f 3 -S 4 & # f=3 mais s=4 → f=s-1
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 6 -f 2 -S 4 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 7 -f 2 -S 3 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 8 -f 1 -S 3 &
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 128 -D "rep-soft8" -o 9 -f 1 -S 2 &
done
wait
echo "Terminé !"