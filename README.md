What can it do?
---------------

**mediabarcode** converts your video files into colorful barcodes. It's heavily inspired by the [moviebarcode tumblr](http://moviebarcode.tumblr.com/movie-index). It takes the video's frames in regular intervals, scales them to 1px width, and appends them. The resulting barcodes can be integrated into video players for simplified navigation.

Installation
------------

If you're using Arch Linux, there's a PKGBUILD in `packages/archlinux`. Otherwise, read on.

You'll need CMake and FFmpeg. Then issue

    mkdir build && cd build && cmake .. && make && make install

Usage
-----

- API documentation: see [src/mediabarcode.h](src/mediabarcode.h)
- Simple usage example: see [src/mediabarcode-bin.c](src/mediabarcode-bin.c)
- Command line tool usage: `mediabarcode videofile [width [height]]` creates `./mediabarcode.ppm`. Size defaults to 1024x400.
