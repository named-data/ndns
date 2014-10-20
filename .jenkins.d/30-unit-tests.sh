#!/usr/bin/env bash
set -x
set -e

# Prepare environment
sudo rm -Rf ~/.ndn

IS_OSX=$( python -c "print 'yes' if 'OSX' in '$NODE_LABELS'.strip().split(' ') else 'no'" )
IS_LINUX=$( python -c "print 'yes' if 'Linux' in '$NODE_LABELS'.strip().split(' ') else 'no'" )

if [ $IS_OSX = "yes" ]; then
    security unlock-keychain -p "named-data"
fi

# Run unit tests
./build/unit-tests -l test_suite
