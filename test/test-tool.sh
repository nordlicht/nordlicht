#!/bin/bash

pass() {
    echo "Running '$@'"
    $@ &> "$LOG"
    exit_code=$?

    if test $exit_code != 0; then
        cat "$LOG"
        echo "command failed: $*"
        exit 1
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

VIDEO='video.mp4'
LOG='test.log'
TMP="tests-tmp_$$/"

mkdir "$TMP" &&
ln -s ../nordlicht "$TMP" &&
ln -s ../"$VIDEO" "$TMP" &&
pushd "$TMP" || exit 1

# test environment
pass true
fail false
pass test -f "$VIDEO"

# argument parsing
pass ./nordlicht --help
fail ./nordlicht
fail ./nordlicht --fubar
fail ./nordlicht "$VIDEO" somethingelse

# input file
fail ./nordlicht nonexistantvideo.foo

# size
pass ./nordlicht "$VIDEO" -w 1 -h 1
fail ./nordlicht "$VIDEO" -w huuuge
fail ./nordlicht "$VIDEO" -w 0
fail ./nordlicht "$VIDEO" -h 0
fail ./nordlicht "$VIDEO" -w ''
fail ./nordlicht "$VIDEO" -h ''
fail ./nordlicht "$VIDEO" -w -100
fail ./nordlicht "$VIDEO" -h -100
fail ./nordlicht "$VIDEO" -w 1.1
fail ./nordlicht "$VIDEO" -h 1.1
fail ./nordlicht "$VIDEO" -w 1,1
fail ./nordlicht "$VIDEO" -h 1,1

# output file
pass ./nordlicht "$VIDEO" -w 1 -o unicode.png
pass ./nordlicht "$VIDEO" -w 1 -o ünîç⌀də.png
pass test -f ünîç⌀də.png
fail ./nordlicht "$VIDEO" -o ../"$TMP"
fail ./nordlicht "$VIDEO" -o "$VIDEO"
fail ./nordlicht "$VIDEO" -o ''

# style
pass ./nordlicht "$VIDEO" -w 1 -s vertical
pass ./nordlicht "$VIDEO" -w 1 -s horizontal
fail ./nordlicht "$VIDEO" -s nope
fail ./nordlicht "$VIDEO" -s ''

# exact seeking
pass ./nordlicht "$VIDEO" -w 1 -e

popd && rm -rv "$TMP" || exit 1

echo "All assertions passed. Yay!"
