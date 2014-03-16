#!/bin/bash

nordlicht() {
    ../nordlicht $@
}

abort() {
    exit 1
}

pass() {
    echo "Running '$@'"
    $@ &> "$LOG"
    exit_code=$?

    if test $exit_code != 0; then
        cat "$LOG"
        echo "command failed: $*"
        abort
    fi
    return 0
}

fail() {
    echo "Running '$@'"
    $@ &> "$LOG"
    exit_code=$?

    if test $exit_code = 0; then
        cat "$LOG"
        echo "command succeded: $*"
        abort
    elif test $exit_code -gt 129 -a $exit_code -le 192; then
        echo "died by signal: $*"
        abort
    elif test $exit_code = 127; then
        echo "command not found: $*"
        abort
    elif test $exit_code = 126; then
        echo "valgrind error: $*"
        abort
    fi
    return 0
}

LOG='test.log'
TMP="test_tmp_$$/"

mkdir "$TMP" &&
pushd "$TMP" || exit 1

VIDEO='../video.mp4'

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
pass nordlicht "$VIDEO" -w 1 -h 10000
fail nordlicht "$VIDEO" -w huuuge
fail nordlicht "$VIDEO" -w 0
fail nordlicht "$VIDEO" -h 0
fail nordlicht "$VIDEO" -w ''
fail nordlicht "$VIDEO" -h ''
fail nordlicht "$VIDEO" -w -100
fail nordlicht "$VIDEO" -h -100
fail nordlicht "$VIDEO" -w 1.1
fail nordlicht "$VIDEO" -h 1.1
fail nordlicht "$VIDEO" -w 1,1
fail nordlicht "$VIDEO" -h 1,1

# output file
pass nordlicht "$VIDEO" -w 1 -o ünîç⌀də.png
pass test -f ünîç⌀də.png
fail nordlicht "$VIDEO" -o ../"$TMP"
fail nordlicht "$VIDEO" -o "$VIDEO"
fail nordlicht "$VIDEO" -o ''

# style
pass nordlicht "$VIDEO" -w 1 -s vertical
pass nordlicht "$VIDEO" -w 1 -s horizontal
fail nordlicht "$VIDEO" -s nope
fail nordlicht "$VIDEO" -s ''

# output formats
pass nordlicht "$VIDEO" -w 1 -o barcode.bgra
fail nordlicht "$VIDEO" -w 1 -o barcode.123
fail nordlicht "$VIDEO" -w 1 -o not-a-png

popd
rm -r "$TMP"

echo "--- nordlicht tool test suite passed. ---"
