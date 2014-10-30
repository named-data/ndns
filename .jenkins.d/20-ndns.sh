#!/usr/bin/env bash
set -x
set -e

./waf distclean --color=yes
./waf configure --debug --with-tests --color=yes
./waf -j1 --color=yes
sudo ./waf install --color=yes
