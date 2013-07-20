Description
-----------

**nordlicht** converts video files into colorful barcodes. It's heavily inspired by the [moviebarcode tumblr](http://moviebarcode.tumblr.com/movie-index). It takes the video's frames in regular intervals, scales them to 1px width, and appends them. The resulting barcodes can be integrated into video players for simplified navigation.

Installation
------------

If you're using Arch Linux, there's a PKGBUILD in `packages/archlinux`.

Otherwise, get CMake and FFmpeg, and issue

    mkdir build && cd build && cmake .. && make && make install

Usage
-----

- API documentation: see [src/nordlicht.h](src/nordlicht.h)
- Simple usage example: see [src/nordlicht-bin.c](src/nordlicht-bin.c)
- Command line tool usage: `nordlicht videofile [width [height]]` creates `./nordlicht.png`. Size defaults to 1024x400.
