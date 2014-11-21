#!/usr/bin/env bash
set -x
set -e

COVERAGE=$( python -c "print '--with-coverage --debug' if 'code-coverage' in '$JOB_NAME' else ''" )

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in debug mode
./waf -j1 --color=yes configure --with-tests --debug
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in optimized mode without tests
./waf -j1 --color=yes configure
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in optimized mode
./waf -j1 --color=yes configure --with-tests $COVERAGE
./waf -j1 --color=yes build

# (tests will be run against optimized version)

# Install
sudo ./waf -j1 --color=yes install
