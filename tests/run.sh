#!/bin/bash

set -e

bins=(build/tests/{multi-client/{tcp,uds,ws,enc},single-client/mock})
for bin in ${bins[@]}; do
    echo "$bin"
    "$bin"
done
echo "all pass"
