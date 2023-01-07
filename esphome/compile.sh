#!/bin/bash

docker run --rm -v "$PWD":/config:delegated -it esphome/esphome:2022.10.2 compile rp2040-connect.yaml