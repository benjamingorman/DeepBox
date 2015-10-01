#!/bin/bash -e

for f in positions/pisquare37*.dbl; do
    cat $f | bin/client -s deepbox -x
    read blah
done
