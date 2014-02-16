#!/bin/sh

pass() {
    $@ &> /tmp/nordlicht_test
    exit_code=$?

    if test $exit_code != 0; then
        cat /tmp/nordlicht_test
        echo "command failed: $*"
        exit 1
    fi
    return 0
}

fail() {
    $@ &> /tmp/nordlicht_test
    exit_code=$?

    if test $exit_code = 0; then
        cat /tmp/nordlicht_test
        echo "command succeded: $*"
        exit 1
    elif test $exit_code -gt 129 -a $exit_code -le 192; then
        echo "died by signal: $*"
        exit 1
    elif test $exit_code = 127; then
        echo "command not found: $*"
        exit 1
    elif test $exit_code = 126; then
        echo "valgrind error: $*"
        exit 1
    fi
    return 0
}

PATH=.:/bin:/usr/bin:/usr/sbin
VIDEO="video.mp4"
OUTPUT="output.png"

# test environment
pass true
fail false
pass test -f "$VIDEO"

# argument parsing
pass nordlicht --help
fail nordlicht
fail nordlicht --fubar
fail nordlicht "$VIDEO" somethingelse

# input file
fail nordlicht nonexistantvideo.foo

# size
pass nordlicht "$VIDEO" -w 1 -h 1
fail nordlicht "$VIDEO" -w huuuge
fail nordlicht "$VIDEO" -w 0
fail nordlicht "$VIDEO" -h 0
fail nordlicht "$VIDEO" -w -100
fail nordlicht "$VIDEO" -h -100

# output file
pass nordlicht "$VIDEO" -w 1 -o unicode.png
pass nordlicht "$VIDEO" -w 1 -o ünîç⌀də.png
pass test -f ünîç⌀də.png
fail nordlicht "$VIDEO" -o /tmp
fail nordlicht "$VIDEO" -o "$VIDEO"
fail nordlicht "$VIDEO" -o ""

# style
pass nordlicht "$VIDEO" -w 1 -s vertical
pass nordlicht "$VIDEO" -w 1 -s horizontal
fail nordlicht "$VIDEO" -s nope

echo "All assertions passed. Yay!"
