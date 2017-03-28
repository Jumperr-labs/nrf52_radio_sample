#!/usr/bin/env bash
pushd $(dirname ${BASH_SOURCE[0]})
cd receiver
make
cd ..
cd transmitter
make
cd ..
popd