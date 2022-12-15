#!/usr/bin/bash

mkdir -p /tmp/examples

./bin/front $1 /tmp/$1.ast                      && \
    ./bin/back  /tmp/$1.ast /tmp/$1.asm         && \
    ./cpu/bin/asm /tmp/$1.asm /tmp/$1.bin       && \
    ./cpu/bin/cpu /tmp/$1.bin