## Description

**nordlicht** is a C library for converting video files into colorful barcodes. It is inspired by the [Moviebarcode Tumblr](http://moviebarcode.tumblr.com/), but aims at the next logical step: Integrating these barcodes into video players to make navigation faster and more precise.

*nordlicht* provides a command line tool, which you can use to generate your own barcodes easily.

## Examples

Here's the barcode for [Tears of Steel](http://tearsofsteel.org/). It was created by taking the movie's frames at regular intervals, scaling them to 1 pixel width, and appending them. You can differentiate inside and outside scenes, the credits, and the "secret" scene at the end:

![](examples/tears-of-steel.png)

This barcode of [Elephants Dream](http://www.elephantsdream.org/) uses the *vertical* style, which compresses video's frames to columns and rotates them counterclockwise. This style emphasizes the movement of characters:

![](examples/elephants-dream-vertical.png)

## Installation

- Arch Linux: Install the [`nordlicht-git`](https://aur.archlinux.org/packages/nordlicht-git/) package from the AUR.
- Gentoo: Install the `media-video/nordlicht` package from the [multimedia overlay](https://gitorious.org/gentoo-multimedia/gentoo-multimedia).
- On other distributions, get CMake, FFmpeg, FreeImage, and [popt](http://freecode.com/projects/popt), and issue: `mkdir build && cd build && cmake .. && make && make install`

## Usage

### Command line tool

Basic usage: `nordlicht video.mkv -w 1000 -h 150 -o barcode.png` converts *video.mkv* to a barcode of 1000 x 150 pixels size and writes it to *barcode.png*. Run `nordlicht --help` to learn more.

### Library

- API documentation: see [src/nordlicht.h](src/nordlicht.h)
- Simple usage example: see [src/main.c](src/main.c)

## Integrations

### mpv

For [mpv](http://mpv.io/), there's a hacky script called [mpv-nordlicht](/utils/mpv-nordlicht). Put it in your `PATH` and use it instead of `mpv`; it will generate a barcode for its last argument, then start mpv in fullscreen mode and display the barcode at the top, along with the OSD progress bar. This is how it looks like (for [Decay](http://www.decayfilm.com/)):

![](/examples/mpv-integration.png)

## License: GPLv2+

*nordlicht* is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
