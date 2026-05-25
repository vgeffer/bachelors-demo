#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: $0 [N] [Benchmark Name] [Model] (aditional args for raytracer)"
    exit -1
fi

if [ -e "./raytracer" ]; then
    EXECUTABLE="./raytracer"
elif [ -e "build/raytracer" ]; then
    EXECUTABLE="build/raytracer"
else
    echo "Raytracer binary not found!"
    exit -1
fi

for ((i = 0 ; i < $1 ; i++)); do
    echo "Starting iteration $i"
    $EXECUTABLE $3 -b 5 > perf-data/$2-$i.csv 2>/dev/null
done