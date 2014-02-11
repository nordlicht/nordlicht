## Description

**nordlicht** is a C library that converts your video files into colorful barcodes. It was inspired by the [moviebarcode tumblr](http://moviebarcode.tumblr.com/). It takes the video's frames in regular intervals, scales them to 1px width, and appends them. You can integrate the resulting barcodes into video players for faster navigation.

Here's the barcode of [Tears of Steel](http://tearsofsteel.org/). You can differentiate inside and outside scenes, see the credits and the closing scene:

![Barcode for "Tears of Steel"](res/tos-example.png)

An experimental [VLC integration](https://github.com/blinry/vlc) exists, that uses *nordlicht* to generate these barcodes on the fly.

## Installation

If you are using Arch Linux, you can install the [**`nordlicht-git`**](https://aur.archlinux.org/packages/nordlicht-git/) package from the AUR.

Otherwise, get CMake, FFmpeg, and FreeImage, and issue

    mkdir build && cd build && cmake .. && make && make install

## Usage

### Command line tool

Run `nordlicht` to get usage instructions for the command line tool.

### Library

- API documentation: see [src/nordlicht.h](src/nordlicht.h)
- Simple usage example: see [src/nordlicht-tool.c](src/nordlicht-tool.c)

## License

*nordlicht* is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
