#!/bin/bash

# set -e

cd $(dirname "$0")
docker run --rm -v "$PWD":/config:delegated -it esphome/esphome:2022.10.2 compile smart-lcc.yaml
./make_headers.sh