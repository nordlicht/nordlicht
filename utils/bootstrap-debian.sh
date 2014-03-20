#!/usr/bin/env bash

apt-get update &&
apt-get install -y cmake libavcodec-dev libswscale-dev libavformat-dev libpng-dev libpopt-dev git-buildpackage debhelper help2man python-software-properties
