#!/usr/bin/env bash

apt-get update &&
apt-get install -y cmake libavcodec-dev libswscale-dev libavformat-dev libfreeimage-dev libpopt-dev git-buildpackage debhelper help2man
