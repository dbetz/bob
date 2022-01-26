#! /bin/bash

chmod +x *.bob

# execute all test functions and catch the out put of each test in a unique file
for i in test_*.bob
do
    zz=$( basename "$i" )

    (
        echo "$i"
        ../bin/bob -v ./$i
    ) |
    tee $zz.txt
done
