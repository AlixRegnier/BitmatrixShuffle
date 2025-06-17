#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: get_permutation.sh <csv>"
    exit 1
fi

if [ ! -e $1 ]; then
    echo "File '$1' doesn't exist !"
    exit 2
fi

name=${1%.csv}

rows=$(wc -l $1 | cut -f1 -d' ')
rows=$(($rows-1))

header=$(head -n 1 $1)
samples="${header//[^,]}"
samples=${#samples}

columns=$(($samples/8*8))

echo -e "Columns:\t$samples\nRows:\t\t$rows" 

echo ":: Call 'csv_to_bin.py'"
python3 /exec/csv_to_bin.py $1 $samples $rows $name.bin

echo ":: Call 'reorder'"
/exec/reorder $name.bin $samples 0 $columns 0 0 $name.col.bin $name.row.bin $name.bin

echo ":: Call 'read_order'"
/exec/read_order $samples $name.col.bin 1> $name.col.txt 
echo ":: Call 'read_order'"
/exec/read_order $rows $name.row.bin 1> $name.row.txt

echo ":: Delete tmp files"
rm $name.col.bin $name.row.bin $name.bin

