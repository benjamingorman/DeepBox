#!/bin/bash -e

for f in positions/*.dbl; do
    cat $f | bin/client -s deepbox1 -x
    read blah
done
