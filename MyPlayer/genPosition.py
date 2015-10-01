#!/usr/bin/env python

import random
import sys

movesToMake = 72 - int(sys.argv[1])

position = list("0" * 72)
for i in random.sample(range(72), movesToMake):
    position[i] = "1"

print("".join(position))
