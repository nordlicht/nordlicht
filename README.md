## Description

**nordlicht** is a C library that converts your video files into colorful barcodes. It was inspired by the [moviebarcode tumblr](http://moviebarcode.tumblr.com/). It takes the video's frames in regular intervals, scales them to 1px width, and appends them. You can integrate the resulting barcodes into video players for faster navigation, see the section below.

As an example, here is the barcode of [Tears of Steel](http://tearsofsteel.org/). You can differentiate inside and outside scenes, see the credits and the "secret" closing scene:

![Barcode for "Tears of Steel"](res/tos-example.png)

## Installation

If you are using Arch Linux, you can install the [**`nordlicht-git`**](https://aur.archlinux.org/packages/nordlicht-git/) package from the AUR.

On Gentoo, install the **`media-video/nordlicht`** package from the [multimedia overlay](https://gitorious.org/gentoo-multimedia/gentoo-multimedia).

Otherwise, get CMake, FFmpeg, and FreeImage, and issue

    mkdir build && cd build && cmake .. && make && make install

## Usage

### Command line tool

Run `nordlicht` to get usage instructions for the command line tool.

### Library

- API documentation: see [src/nordlicht.h](src/nordlicht.h)
- Simple usage example: see [src/nordlicht-tool.c](src/nordlicht-tool.c)

## Integrations

### mpv

Are you using a recent [mpv](http://mpv.io/)? Would you like to try how navigating a video with a *nordlicht* feels like? Do you fancy hacky scripts? [mpv-nordlicht](/res/mpv-nordlicht) is for you! It generates a barcode for its only argument, then starts mpv and sets up the keybindings `n`/`N` to display/hide the barcode at the top of the video using mpv's `overlay_add` command. The OSD progress bar (which is displayed when using the arrow keys) is positioned below that. The barcode is adapted to your monitor resolution, so please activate fullscreen.

### VLC

An experimental [VLC integration](https://github.com/blinry/vlc) exists, that uses *nordlicht* to generate the barcode on the fly and display it in the main seek slider. Currently it needs to be updated to the 0.2 API.

## License

*nordlicht* is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
