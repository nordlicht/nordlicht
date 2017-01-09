# Change Log

All notable changes to this project will be documented in this file.
*nordlicht* uses [Semantic Versioning](http://semver.org/).

## 0.4.5 - 2017-01-09

- introduce `nordlicht_generate_step` and `nordlicht_done` to the API, for simple, non-blocking applications
- improve FFmpeg backwards compability down to v3.0 (thanks, Manuel!)
- fix build on ARM platforms
- correctly open files which contain a colon (thanks, Roland!)
- various bug fixes and stability improvements

- tool: when the user specifies a non-PNG file as output, warn them but don't fail

## 0.4.4 - 2016-01-24

- introduce the convention that the nordlicht is always written to `VIDEOFILE.nordlicht.png`
- increase default size to 1920x192, to get nice results on 1080p displays
- introduce `nordlicht_set_style`, which only accepts one style
- rename old `nordlicht_set_style` to `nordlicht_set_styles`
- fixes to the build system: don't over-/underlink (thanks, Roland!), set library `VERSION` only export symbols of the public API
- make pkg-config file more specific (thanks, Roland!)

- mpv: check whether this file exists before generating a new one
- mpv: press *N* to re-generate nordlicht
- mpv: show nordlicht for 1 second when navigating in "off" mode (thanks, Roland!)

## 0.4.3 - 2016-01-17

- add LICENSE.md and CHANGELOG.md
- add export specifier to API functions (thanks, Peter!)
- improve documentation at various places
- mpv plugin: Seek when clicking into the nordlicht

## 0.4.2 - 2016-01-02

- report the correct version number when using `--version`. Oops.

## 0.4.1 - 2015-12-11

Minor fix for better packaging:

- don't test for unused FFmpeg libraries in CMake. Thanks, Olaf!

## 0.4.0 "LED Throwie" - 2015-10-05

- introduce multiple new styles, including slitscan, middlecolumn and spectrogram
- allow specification of multiple styles at once, using `-s style1+style2`
- introduce options 'start' end 'end' in API and tool
- introduce `nordlicht_error()`, which returns a description of the last error
- add a `--quiet` switch to the tool, that suppresses progress output
- use FFmpeg to write PNGs, drop libpng dependency
- various bug fixes for older FFmpeg versions

## 0.3.5 - 2014-03-20

- add a test suite for library and tool, fix a few bugs along the way
- add examples for the C API
- introduce `nordlicht_set_style()` to replace the "live" switch

## 0.3.4 - 2014-03-15

This is a bug fix release:

- fix a bug in upscaling the barcode's height
- don't `exit()` in library calls

## 0.3.3 - 2014-03-03

Quick release to enable better packeting:

- Replace FreeImage by libpng.

## 0.3.2 - 2014-03-02

- generate a Makefile from `--help` using help2man
- do proper SOVERSION-ing
- introduce an experimental `--live` mode, which generates a BGRA file for mpv
- the mpv integration is really coming along

## 0.3.1 - 2014-02-23

- Automatically decide whether exact seeking is necessary, remove the manual switch
- Significant quality improvements by better vertical and horizontal compression
- Fix time shift by correctly flushing FFmpeg's buffers
- Improve mpv-nordlicht to automatically display the barcode and go fullscreen

## 0.3.0 "Blaze" - 2014-02-16

- New CLI API with proper GNU-style arguments
- Option to do "vertical" compression: `--style=vertical`
- Experimental "frame exakt" seeking with `--exact` switch
- Hacked together an mpv-nordlicht script that generates a barcode and displays it in mpv
- Fixed some memory leaks, added a lot of security checks

## 0.2.0 "Spark" - 2014-02-11

Features a complete rewrite, removing the strong dependency on FFmpeg. Additional improvements:

- Hand written downsampling, making generation much faster
- New, simpler API
- Command line tool now checks it's arguments

## 0.1.1 - 2013-08-04

A minor release, mainly patching the code for older FFmpeg versions.

## 0.1.0 "Fiat Lux" - 2013-08-02

After a few prototypes in shell script (look at the history if you're curious),
this is the first "stable" version of nordlicht. It comes with a command line
tool which uses the shared library.
