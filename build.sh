#!/bin/bash

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DBMS_BUILD_MAIN=true
make

cd -
