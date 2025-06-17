#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: get_permutation.sh <csv> <columns> <rows>"
    exit 1
fi

if [ ! -e $1 ]; then
    echo "File '$1' doesn't exist !"
    exit 2
fi

name=${1%.csv}
columns=$((($2 + 7)/8*8))

echo ":: Call 'csv_to_bin.py'"
python3 /exec/csv_to_bin.py $@ $name.bin

echo ":: Call 'reorder'"
reorder $name.bin $2 0 $columns 0 0 $name.col.bin $name.row.bin $name.bin

echo ":: Call 'read_order'"
read_order $2 $name.col.bin 1> $name.col.txt 
echo ":: Call 'read_order'"
read_order $3 $name.row.bin 1> $name.row.txt

echo ":: Delete tmp files"
rm $name.col.bin $name.row.bin $name.bin

