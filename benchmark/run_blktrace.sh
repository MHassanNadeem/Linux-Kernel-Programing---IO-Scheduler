#!/bin/bash

# -d: device
# -i: trace only issued request

sudo blktrace -d /dev/sda -a issue -o - | blkparse -i -
